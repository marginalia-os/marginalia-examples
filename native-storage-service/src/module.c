#include <stdint.h>

#include "marginalia/native_abi_v1.h"

typedef struct StorageServiceState {
  const MarginaliaNativeHostV1* host;
  uint32_t job_slot;
  uint32_t job_generation;
  uint32_t request_pending;
  uint32_t write_after_read;
  uint32_t finished;
  uint8_t marker[16];
} StorageServiceState;

static const char kModuleName[] = "Native Storage Service";
static const char kComponentId[] = "storage-service";
static const char kStatePath[] = "state.bin";
static const uint8_t kVersionMarker[] = "storage-v1";

static void zero_bytes(uint8_t* bytes, uint32_t count) {
  uint32_t index;
  for (index = 0; index < count; ++index) bytes[index] = 0;
}

static void set_header(MarginaliaNativeAbiHeader* header, uint32_t size) {
  header->size = size;
  header->abi_major = MARGINALIA_NATIVE_ABI_MAJOR;
  header->abi_minor = MARGINALIA_NATIVE_ABI_MINOR;
}

static MarginaliaNativeStatus storage_create(const MarginaliaNativeHostV1* host,
                                             const MarginaliaNativeContextV1* context, void** output_state) {
  StorageServiceState* state;
  uint32_t index;

  (void)context;
  if (host == 0 || host->alloc == 0 || output_state == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  state = (StorageServiceState*)host->alloc(host, (uint32_t)sizeof(StorageServiceState));
  if (state == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  zero_bytes((uint8_t*)state, (uint32_t)sizeof(StorageServiceState));
  state->host = host;
  for (index = 0; index < (uint32_t)sizeof(kVersionMarker) - 1U; ++index) state->marker[index] = kVersionMarker[index];
  *output_state = state;
  return MARGINALIA_NATIVE_STATUS_OK;
}

static MarginaliaNativeStatus storage_start(void* opaque_state) {
  StorageServiceState* state = (StorageServiceState*)opaque_state;
  if (state == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  state->request_pending = 0;
  state->write_after_read = 0;
  state->finished = 0;
  return MARGINALIA_NATIVE_STATUS_OK;
}

static MarginaliaNativeStatus begin_request(StorageServiceState* state, uint32_t operation, uint32_t bytes,
                                            const uint8_t* input, uint32_t input_size) {
  MarginaliaNativeStorageRequestV1 request;
  MarginaliaNativeStatus status;

  zero_bytes((uint8_t*)&request, (uint32_t)sizeof(request));
  set_header(&request.header, (uint32_t)sizeof(request));
  request.name_space = MARGINALIA_NATIVE_STORAGE_NAMESPACE_DATA;
  request.operation = operation;
  request.offset = 0;
  request.bytes = bytes;
  request.path.data = kStatePath;
  request.path.size = (uint32_t)sizeof(kStatePath) - 1U;
  request.input = input;
  request.input_size = input_size;
  status = state->host->storage_begin(state->host, &request, &state->job_slot, &state->job_generation);
  if (status == MARGINALIA_NATIVE_STATUS_OK) state->request_pending = 1;
  return status;
}

static MarginaliaNativeStatus storage_tick(void* opaque_state, const MarginaliaNativeTickV1* tick,
                                            MarginaliaNativeTickResultV1* result) {
  StorageServiceState* state = (StorageServiceState*)opaque_state;
  MarginaliaNativeStatus status;
  uint32_t operation;
  const uint8_t* input;
  uint32_t input_size;

  if (state == 0 || tick == 0 || result == 0 || state->host == 0 || state->host->storage_begin == 0) {
    return MARGINALIA_NATIVE_STATUS_ERROR;
  }
  set_header(&result->header, (uint32_t)sizeof(*result));
  result->status = MARGINALIA_NATIVE_STATUS_OK;
  result->next_wake_ms = tick->now_ms + 1000U;
  if (state->request_pending || state->finished) {
    result->next_wake_ms = tick->now_ms + 1000U;
    return MARGINALIA_NATIVE_STATUS_OK;
  }

  operation = state->write_after_read ? MARGINALIA_NATIVE_STORAGE_OPERATION_WRITE
                                       : MARGINALIA_NATIVE_STORAGE_OPERATION_READ;
  input = state->write_after_read ? state->marker : 0;
  input_size = state->write_after_read ? (uint32_t)sizeof(state->marker) : 0U;
  status = begin_request(state, operation, (uint32_t)sizeof(state->marker), input, input_size);
  if (status == MARGINALIA_NATIVE_STATUS_OK) {
    result->next_wake_ms = tick->now_ms;
    return MARGINALIA_NATIVE_STATUS_OK;
  }
  if (status == MARGINALIA_NATIVE_STATUS_RETRY) {
    result->next_wake_ms = tick->now_ms + 100U;
    return MARGINALIA_NATIVE_STATUS_OK;
  }
  return status;
}

static MarginaliaNativeStatus storage_event(void* opaque_state, const MarginaliaNativeEventV1* event) {
  StorageServiceState* state = (StorageServiceState*)opaque_state;
  const MarginaliaNativeStorageEventV1* storage;

  if (state == 0 || event == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  if (event->type != MARGINALIA_NATIVE_EVENT_PACKAGE_STORAGE_V1) return MARGINALIA_NATIVE_STATUS_OK;
  if (event->data == 0 || event->data_size < (uint32_t)sizeof(MarginaliaNativeStorageEventV1)) {
    return MARGINALIA_NATIVE_STATUS_ERROR;
  }
  storage = (const MarginaliaNativeStorageEventV1*)event->data;
  if (storage->job_slot != state->job_slot || storage->job_generation != state->job_generation) {
    return MARGINALIA_NATIVE_STATUS_ERROR;
  }
  state->request_pending = 0;
  if (storage->kind == MARGINALIA_NATIVE_STORAGE_EVENT_COMPLETED) {
    if (storage->operation == MARGINALIA_NATIVE_STORAGE_OPERATION_READ) {
      state->write_after_read = 0;
      state->finished = 1;
    } else if (storage->operation == MARGINALIA_NATIVE_STORAGE_OPERATION_WRITE) {
      state->finished = 1;
    }
  } else if (storage->kind == MARGINALIA_NATIVE_STORAGE_EVENT_FAILED &&
             storage->operation == MARGINALIA_NATIVE_STORAGE_OPERATION_READ &&
             storage->error == MARGINALIA_NATIVE_STORAGE_ERROR_NOT_FOUND) {
    state->write_after_read = 1;
  } else if (storage->kind == MARGINALIA_NATIVE_STORAGE_EVENT_CANCELLED) {
    state->finished = 1;
  } else {
    return MARGINALIA_NATIVE_STATUS_ERROR;
  }
  return MARGINALIA_NATIVE_STATUS_OK;
}

static void storage_stop(void* opaque_state, MarginaliaNativeStopReason reason) {
  StorageServiceState* state = (StorageServiceState*)opaque_state;
  (void)reason;
  if (state == 0 || !state->request_pending || state->host == 0 || state->host->storage_cancel == 0) return;
  (void)state->host->storage_cancel(state->host, state->job_slot, state->job_generation,
                                    MARGINALIA_NATIVE_STOP_DISABLE);
  state->request_pending = 0;
}

static void storage_destroy(void* opaque_state) {
  StorageServiceState* state = (StorageServiceState*)opaque_state;
  if (state != 0 && state->host != 0 && state->host->free != 0) state->host->free(state->host, state);
}

static MarginaliaNativeModuleDescriptorV1 kDescriptor = {
    .header = {sizeof(MarginaliaNativeModuleDescriptorV1), MARGINALIA_NATIVE_ABI_MAJOR, MARGINALIA_NATIVE_ABI_MINOR},
    .module_name = {kModuleName, sizeof(kModuleName) - 1U},
    .component_id = {kComponentId, sizeof(kComponentId) - 1U},
    .role = MARGINALIA_NATIVE_ROLE_SERVICE,
    .resources = {
        .header = {sizeof(MarginaliaNativeResourceLimitsV1), MARGINALIA_NATIVE_ABI_MAJOR,
                   MARGINALIA_NATIVE_ABI_MINOR},
        .heap_bytes = 2048U,
        .frame_bytes = 0U,
        .asset_read_bytes = 2048U,
        .storage_bytes = 2048U,
        .callback_budget_ms = 50U,
    },
    .callbacks = {
        .header = {sizeof(MarginaliaNativeCallbacksV1), MARGINALIA_NATIVE_ABI_MAJOR, MARGINALIA_NATIVE_ABI_MINOR},
        .create = storage_create,
        .start = storage_start,
        .tick = storage_tick,
        .event = storage_event,
        .prepare_frame = 0,
        .stop = storage_stop,
        .destroy = storage_destroy,
    },
};

__attribute__((visibility("default"))) const MarginaliaNativeModuleDescriptorV1* marginalia_module_entry_v1(void) {
  return &kDescriptor;
}
