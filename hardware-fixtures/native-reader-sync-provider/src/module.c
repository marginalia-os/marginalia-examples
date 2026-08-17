#include <stdint.h>

#include "marginalia/native_abi_v1.h"

typedef struct ReaderSyncProviderState {
  const MarginaliaNativeHostV1* host;
  uint32_t job_slot;
  uint32_t job_generation;
  uint32_t operation;
  uint32_t stage;
  uint32_t active;
} ReaderSyncProviderState;

static const char kModuleName[] = "Native Reader Sync Provider Fixture";
static const char kComponentId[] = "reader-sync-provider";
static const char kRemoteAnchor[] = "fixture-remote-anchor";
static const char kDeviceId[] = "fixture-device";

#define PROVIDER_STAGE_IDLE 0U
#define PROVIDER_STAGE_PENDING 1U
#define PROVIDER_STAGE_PROGRESS 2U
#define PROVIDER_STAGE_TERMINAL 3U

static void zero_bytes(uint8_t* bytes, uint32_t count) {
  uint32_t index;
  for (index = 0; index < count; ++index) bytes[index] = 0;
}

static void set_header(MarginaliaNativeAbiHeader* header, uint32_t size) {
  header->size = size;
  header->abi_major = MARGINALIA_NATIVE_ABI_MAJOR;
  header->abi_minor = MARGINALIA_NATIVE_ABI_MINOR;
}

static uint32_t valid_header(const MarginaliaNativeAbiHeader* header, uint32_t minimum_size) {
  return header != 0 && header->size >= minimum_size && header->abi_major == MARGINALIA_NATIVE_ABI_MAJOR &&
         header->abi_minor <= MARGINALIA_NATIVE_ABI_MINOR;
}

static uint32_t copy_bytes(char* destination, uint32_t capacity, const char* source) {
  uint32_t count = 0;
  if (destination == 0 || source == 0 || capacity == 0) return 0;
  while (count < capacity && source[count] != '\0') {
    destination[count] = source[count];
    count += 1U;
  }
  return count;
}

static uint32_t now_ms(const ReaderSyncProviderState* state) {
  if (state != 0 && state->host != 0 && state->host->now_ms != 0) return state->host->now_ms(state->host);
  return 0U;
}

static MarginaliaNativeStatus emit_event(ReaderSyncProviderState* state, uint32_t kind, uint32_t phase, uint32_t error,
                                         uint32_t next_wake_ms, uint32_t percentage_ppm, uint32_t include_position) {
  MarginaliaNativeReaderSyncEventV1 response;

  if (state == 0 || state->host == 0 || state->host->provider_emit == 0 || state->active == 0) {
    return MARGINALIA_NATIVE_STATUS_ERROR;
  }

  zero_bytes((uint8_t*)&response, (uint32_t)sizeof(response));
  set_header(&response.header, (uint32_t)sizeof(response));
  response.job_slot = state->job_slot;
  response.job_generation = state->job_generation;
  response.kind = kind;
  response.phase = phase;
  response.error = error;
  response.next_wake_ms = next_wake_ms;
  response.percentage_ppm = percentage_ppm;

  if (include_position != 0U) {
    response.spine_index = 1U;
    response.page_number = 2U;
    response.total_pages = 10U;
    response.paragraph_index = 3U;
    response.has_paragraph_index = 1U;
    response.timestamp = 1700000000LL;
    response.anchor_bytes = copy_bytes(response.anchor, (uint32_t)sizeof(response.anchor), kRemoteAnchor);
    response.device_id_bytes = copy_bytes(response.device_id, (uint32_t)sizeof(response.device_id), kDeviceId);
  }

  return state->host->provider_emit(state->host, MARGINALIA_NATIVE_PROVIDER_EVENT_READER_SYNC_V1,
                                    (const uint8_t*)&response, (uint32_t)sizeof(response));
}

static MarginaliaNativeStatus provider_create(const MarginaliaNativeHostV1* host,
                                              const MarginaliaNativeContextV1* context, void** output_state) {
  ReaderSyncProviderState* state;

  (void)context;
  if (host == 0 || host->alloc == 0 || output_state == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  state = (ReaderSyncProviderState*)host->alloc(host, (uint32_t)sizeof(ReaderSyncProviderState));
  if (state == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  zero_bytes((uint8_t*)state, (uint32_t)sizeof(ReaderSyncProviderState));
  state->host = host;
  *output_state = state;
  return MARGINALIA_NATIVE_STATUS_OK;
}

static MarginaliaNativeStatus provider_start(void* opaque_state) {
  ReaderSyncProviderState* state = (ReaderSyncProviderState*)opaque_state;
  if (state == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  state->job_slot = 0;
  state->job_generation = 0;
  state->operation = 0;
  state->stage = PROVIDER_STAGE_IDLE;
  state->active = 0;
  return MARGINALIA_NATIVE_STATUS_OK;
}

static uint32_t valid_request(const MarginaliaNativeReaderSyncRequestV1* request) {
  if (request == 0 || !valid_header(&request->header, (uint32_t)sizeof(*request)) || request->job_generation == 0U ||
      request->document_id_bytes > (uint32_t)sizeof(request->document_id) ||
      request->anchor_bytes > (uint32_t)sizeof(request->anchor) ||
      request->filename_bytes > (uint32_t)sizeof(request->filename) ||
      request->title_bytes > (uint32_t)sizeof(request->title) ||
      request->authors_bytes > (uint32_t)sizeof(request->authors)) {
    return 0U;
  }
  if (request->operation != MARGINALIA_NATIVE_READER_SYNC_OPERATION_FETCH_REMOTE &&
      request->operation != MARGINALIA_NATIVE_READER_SYNC_OPERATION_UPLOAD_LOCAL &&
      request->operation != MARGINALIA_NATIVE_READER_SYNC_OPERATION_CANCEL) {
    return 0U;
  }
  if (request->operation == MARGINALIA_NATIVE_READER_SYNC_OPERATION_CANCEL &&
      (request->cancel_reason < MARGINALIA_NATIVE_STOP_USER ||
       request->cancel_reason > MARGINALIA_NATIVE_STOP_FIRMWARE)) {
    return 0U;
  }
  return 1U;
}

static MarginaliaNativeStatus provider_event(void* opaque_state, const MarginaliaNativeEventV1* event) {
  ReaderSyncProviderState* state = (ReaderSyncProviderState*)opaque_state;
  const MarginaliaNativeReaderSyncRequestV1* request;
  MarginaliaNativeStatus status;

  if (state == 0 || event == 0 || !valid_header(&event->header, (uint32_t)sizeof(*event))) {
    return MARGINALIA_NATIVE_STATUS_ERROR;
  }
  if (event->type != MARGINALIA_NATIVE_EVENT_READER_SYNC_REQUEST_V1 || event->data == 0 ||
      event->data_size < (uint32_t)sizeof(MarginaliaNativeReaderSyncRequestV1)) {
    return MARGINALIA_NATIVE_STATUS_ERROR;
  }

  request = (const MarginaliaNativeReaderSyncRequestV1*)event->data;
  if (!valid_request(request)) return MARGINALIA_NATIVE_STATUS_ERROR;

  if (request->operation == MARGINALIA_NATIVE_READER_SYNC_OPERATION_CANCEL) {
    if (state->active == 0U || state->job_slot != request->job_slot || state->job_generation != request->job_generation) {
      return MARGINALIA_NATIVE_STATUS_ERROR;
    }
    state->stage = PROVIDER_STAGE_TERMINAL;
    status = emit_event(state, MARGINALIA_NATIVE_READER_SYNC_EVENT_CANCELLED,
                        MARGINALIA_NATIVE_READER_SYNC_PHASE_FINISHED,
                        MARGINALIA_NATIVE_READER_SYNC_ERROR_CANCELLED, UINT32_MAX, 0U, 0U);
    return status;
  }

  if (state->active != 0U) return MARGINALIA_NATIVE_STATUS_ERROR;
  state->job_slot = request->job_slot;
  state->job_generation = request->job_generation;
  state->operation = request->operation;
  state->stage = PROVIDER_STAGE_PENDING;
  state->active = 1U;

  status = emit_event(state, MARGINALIA_NATIVE_READER_SYNC_EVENT_PENDING,
                      request->operation == MARGINALIA_NATIVE_READER_SYNC_OPERATION_FETCH_REMOTE
                          ? MARGINALIA_NATIVE_READER_SYNC_PHASE_CONNECTING
                          : MARGINALIA_NATIVE_READER_SYNC_PHASE_UPLOADING,
                      MARGINALIA_NATIVE_READER_SYNC_ERROR_NONE, now_ms(state) + 100U, 0U, 0U);
  if (status != MARGINALIA_NATIVE_STATUS_OK) {
    state->stage = PROVIDER_STAGE_IDLE;
    state->active = 0U;
  }
  return status;
}

static MarginaliaNativeStatus provider_tick(void* opaque_state, const MarginaliaNativeTickV1* tick,
                                            MarginaliaNativeTickResultV1* result) {
  ReaderSyncProviderState* state = (ReaderSyncProviderState*)opaque_state;
  MarginaliaNativeStatus status;
  uint32_t next_wake_ms;

  if (state == 0 || tick == 0 || result == 0 || state->host == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  if (!valid_header(&tick->header, (uint32_t)sizeof(*tick))) return MARGINALIA_NATIVE_STATUS_ERROR;

  set_header(&result->header, (uint32_t)sizeof(*result));
  result->status = MARGINALIA_NATIVE_STATUS_OK;
  result->next_wake_ms = UINT32_MAX;
  if (state->active == 0U || state->stage == PROVIDER_STAGE_TERMINAL) return MARGINALIA_NATIVE_STATUS_OK;

  next_wake_ms = tick->now_ms + 100U;
  if (state->stage == PROVIDER_STAGE_PENDING &&
      state->operation == MARGINALIA_NATIVE_READER_SYNC_OPERATION_FETCH_REMOTE) {
    state->stage = PROVIDER_STAGE_PROGRESS;
    status = emit_event(state, MARGINALIA_NATIVE_READER_SYNC_EVENT_REMOTE_PROGRESS,
                        MARGINALIA_NATIVE_READER_SYNC_PHASE_FETCHING, MARGINALIA_NATIVE_READER_SYNC_ERROR_NONE,
                        next_wake_ms, 500000U, 1U);
  } else {
    state->stage = PROVIDER_STAGE_TERMINAL;
    status = emit_event(state, MARGINALIA_NATIVE_READER_SYNC_EVENT_COMPLETED,
                        MARGINALIA_NATIVE_READER_SYNC_PHASE_FINISHED, MARGINALIA_NATIVE_READER_SYNC_ERROR_NONE,
                        UINT32_MAX, state->operation == MARGINALIA_NATIVE_READER_SYNC_OPERATION_FETCH_REMOTE
                                        ? 1000000U
                                        : 0U,
                        state->operation == MARGINALIA_NATIVE_READER_SYNC_OPERATION_FETCH_REMOTE ? 1U : 0U);
  }
  if (status != MARGINALIA_NATIVE_STATUS_OK) return status;
  result->next_wake_ms = state->stage == PROVIDER_STAGE_TERMINAL ? UINT32_MAX : next_wake_ms;
  return MARGINALIA_NATIVE_STATUS_OK;
}

static void provider_stop(void* opaque_state, MarginaliaNativeStopReason reason) {
  ReaderSyncProviderState* state = (ReaderSyncProviderState*)opaque_state;
  (void)reason;
  if (state == 0) return;
  state->stage = PROVIDER_STAGE_IDLE;
  state->active = 0U;
}

static void provider_destroy(void* opaque_state) {
  ReaderSyncProviderState* state = (ReaderSyncProviderState*)opaque_state;
  if (state != 0 && state->host != 0 && state->host->free != 0) state->host->free(state->host, state);
}

static MarginaliaNativeModuleDescriptorV1 kDescriptor = {
    .header = {sizeof(MarginaliaNativeModuleDescriptorV1), MARGINALIA_NATIVE_ABI_MAJOR, MARGINALIA_NATIVE_ABI_MINOR},
    .module_name = {kModuleName, sizeof(kModuleName) - 1U},
    .component_id = {kComponentId, sizeof(kComponentId) - 1U},
    .role = MARGINALIA_NATIVE_ROLE_PROVIDER,
    .resources = {
        .header = {sizeof(MarginaliaNativeResourceLimitsV1), MARGINALIA_NATIVE_ABI_MAJOR,
                   MARGINALIA_NATIVE_ABI_MINOR},
        .heap_bytes = 2048U,
        .frame_bytes = 0U,
        .asset_read_bytes = 0U,
        .storage_bytes = 0U,
        .callback_budget_ms = 50U,
    },
    .callbacks = {
        .header = {sizeof(MarginaliaNativeCallbacksV1), MARGINALIA_NATIVE_ABI_MAJOR, MARGINALIA_NATIVE_ABI_MINOR},
        .create = provider_create,
        .start = provider_start,
        .tick = provider_tick,
        .event = provider_event,
        .prepare_frame = 0,
        .stop = provider_stop,
        .destroy = provider_destroy,
    },
};

__attribute__((visibility("default"))) const MarginaliaNativeModuleDescriptorV1* marginalia_module_entry_v1(void) {
  return &kDescriptor;
}
