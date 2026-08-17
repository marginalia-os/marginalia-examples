#include <stdint.h>

#include "marginalia/native_abi_v1.h"

typedef struct SmokeAppState {
  const MarginaliaNativeHostV1* host;
  uint32_t ticks;
  uint32_t input_count;
  uint32_t exit_requested;
} SmokeAppState;

static const char kModuleName[] = "Native Smoke App Fixture";
static const char kComponentId[] = "smoke-app";

static void zero_bytes(uint8_t* bytes, uint32_t count) {
  uint32_t index;
  for (index = 0; index < count; ++index) bytes[index] = 0;
}

static void set_header(MarginaliaNativeAbiHeader* header, uint32_t size) {
  header->size = size;
  header->abi_major = MARGINALIA_NATIVE_ABI_MAJOR;
  header->abi_minor = MARGINALIA_NATIVE_ABI_MINOR;
}

static MarginaliaNativeStatus smoke_create(const MarginaliaNativeHostV1* host,
                                           const MarginaliaNativeContextV1* context, void** output_state) {
  SmokeAppState* state;

  (void)context;
  if (host == 0 || host->alloc == 0 || output_state == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  state = (SmokeAppState*)host->alloc(host, (uint32_t)sizeof(SmokeAppState));
  if (state == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  zero_bytes((uint8_t*)state, (uint32_t)sizeof(SmokeAppState));
  state->host = host;
  *output_state = state;
  return MARGINALIA_NATIVE_STATUS_OK;
}

static MarginaliaNativeStatus smoke_start(void* opaque_state) {
  SmokeAppState* state = (SmokeAppState*)opaque_state;
  if (state == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  state->ticks = 0;
  state->input_count = 0;
  state->exit_requested = 0;
  return MARGINALIA_NATIVE_STATUS_OK;
}

static MarginaliaNativeStatus smoke_tick(void* opaque_state, const MarginaliaNativeTickV1* tick,
                                         MarginaliaNativeTickResultV1* result) {
  SmokeAppState* state = (SmokeAppState*)opaque_state;
  if (state == 0 || tick == 0 || result == 0 || state->host == 0) return MARGINALIA_NATIVE_STATUS_ERROR;

  set_header(&result->header, (uint32_t)sizeof(*result));
  result->status = state->exit_requested ? MARGINALIA_NATIVE_STATUS_STOP : MARGINALIA_NATIVE_STATUS_OK;
  result->next_wake_ms = tick->now_ms + 250U;
  state->ticks += 1U;
  if (state->host->request_redraw != 0) (void)state->host->request_redraw(state->host);
  return result->status;
}

static MarginaliaNativeStatus smoke_event(void* opaque_state, const MarginaliaNativeEventV1* event) {
  SmokeAppState* state = (SmokeAppState*)opaque_state;
  const MarginaliaNativeInputEventV1* input;

  if (state == 0 || event == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  if (event->type != MARGINALIA_NATIVE_EVENT_INPUT_V1) return MARGINALIA_NATIVE_STATUS_OK;
  if (event->data == 0 || event->data_size < (uint32_t)sizeof(MarginaliaNativeInputEventV1)) {
    return MARGINALIA_NATIVE_STATUS_ERROR;
  }
  input = (const MarginaliaNativeInputEventV1*)event->data;
  if (input->header.size < (uint32_t)sizeof(*input) || input->header.abi_major != MARGINALIA_NATIVE_ABI_MAJOR ||
      input->header.abi_minor > MARGINALIA_NATIVE_ABI_MINOR) {
    return MARGINALIA_NATIVE_STATUS_ERROR;
  }
  if (input->phase == MARGINALIA_NATIVE_INPUT_PHASE_PRESSED) {
    state->input_count += 1U;
    if (input->button == MARGINALIA_NATIVE_INPUT_BUTTON_POWER) state->exit_requested = 1U;
  }
  if (state->host->request_redraw != 0) (void)state->host->request_redraw(state->host);
  return MARGINALIA_NATIVE_STATUS_OK;
}

static MarginaliaNativeStatus smoke_prepare_frame(void* opaque_state, MarginaliaNativeFrameV1* frame) {
  SmokeAppState* state = (SmokeAppState*)opaque_state;
  const MarginaliaNativeFrameTargetV1* target;
  uint32_t row;
  uint32_t column;
  uint32_t black_column;

  if (state == 0 || frame == 0 || frame->opaque == 0 || frame->data == 0) return MARGINALIA_NATIVE_STATUS_ERROR;
  target = (const MarginaliaNativeFrameTargetV1*)frame->opaque;
  if (target->header.size < (uint32_t)sizeof(*target) || target->header.abi_major != MARGINALIA_NATIVE_ABI_MAJOR ||
      target->header.abi_minor > MARGINALIA_NATIVE_ABI_MINOR ||
      target->format != MARGINALIA_NATIVE_FRAME_FORMAT_MONO_1BPP_V1 || target->bytes == 0 ||
      target->stride_bytes == 0 || target->bytes > frame->capacity) {
    return MARGINALIA_NATIVE_STATUS_ERROR;
  }

  for (column = 0; column < target->bytes; ++column) frame->data[column] = 0xFFU;
  black_column = (state->input_count + state->ticks) % target->stride_bytes;
  for (row = 0; row < target->height; ++row) {
    const uint32_t row_offset = row * target->stride_bytes;
    frame->data[row_offset] = 0x00U;
    frame->data[row_offset + target->stride_bytes - 1U] = 0x00U;
    frame->data[row_offset + black_column] = 0x00U;
  }
  for (column = 0; column < target->stride_bytes; ++column) {
    frame->data[column] = 0x00U;
    frame->data[target->bytes - target->stride_bytes + column] = 0x00U;
  }
  frame->used = target->bytes;
  return MARGINALIA_NATIVE_STATUS_OK;
}

static void smoke_stop(void* opaque_state, MarginaliaNativeStopReason reason) {
  SmokeAppState* state = (SmokeAppState*)opaque_state;
  (void)reason;
  if (state != 0) state->exit_requested = 1U;
}

static void smoke_destroy(void* opaque_state) {
  SmokeAppState* state = (SmokeAppState*)opaque_state;
  if (state != 0 && state->host != 0 && state->host->free != 0) state->host->free(state->host, state);
}

static MarginaliaNativeModuleDescriptorV1 kDescriptor = {
    .header = {sizeof(MarginaliaNativeModuleDescriptorV1), MARGINALIA_NATIVE_ABI_MAJOR, MARGINALIA_NATIVE_ABI_MINOR},
    .module_name = {kModuleName, sizeof(kModuleName) - 1U},
    .component_id = {kComponentId, sizeof(kComponentId) - 1U},
    .role = MARGINALIA_NATIVE_ROLE_APP,
    .resources = {
        .header = {sizeof(MarginaliaNativeResourceLimitsV1), MARGINALIA_NATIVE_ABI_MAJOR,
                   MARGINALIA_NATIVE_ABI_MINOR},
        .heap_bytes = 2048U,
        .frame_bytes = 64U * 1024U,
        .asset_read_bytes = 2048U,
        .storage_bytes = 2048U,
        .callback_budget_ms = 50U,
    },
    .callbacks = {
        .header = {sizeof(MarginaliaNativeCallbacksV1), MARGINALIA_NATIVE_ABI_MAJOR, MARGINALIA_NATIVE_ABI_MINOR},
        .create = smoke_create,
        .start = smoke_start,
        .tick = smoke_tick,
        .event = smoke_event,
        .prepare_frame = smoke_prepare_frame,
        .stop = smoke_stop,
        .destroy = smoke_destroy,
    },
};

__attribute__((visibility("default"))) const MarginaliaNativeModuleDescriptorV1* marginalia_module_entry_v1(void) {
  return &kDescriptor;
}
