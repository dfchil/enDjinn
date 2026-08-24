#include <enDjinn/enj_defs.h>
#include <enDjinn/enj_rumble.h>

#ifdef __DREAMCAST__
#include <dc/maple/purupuru.h>
#endif

static int enj_rumble_rate_limit = 1;
static maple_device_t *local_rumbles[MAPLE_PORT_COUNT] = {0};
static int rumble_rate_limits[MAPLE_PORT_COUNT] = {0};
static uint32_t pending_rumble_effects[MAPLE_PORT_COUNT] = {0};

static inline void scan_local_rumblers(maple_device_t *__unused,
                                       void *__unused) {
  for (int i = 0; i < 4; i++) {
    local_rumbles[i] = NULL;
  }
  int i = 0;
  maple_device_t *purupuru;
  while ((purupuru = maple_enum_type(i, MAPLE_FUNC_PURUPURU))) {
    local_rumbles[purupuru->port] = purupuru;
    i++;
  }
}

void enj_rumble_init_local_devices(void) {
  scan_local_rumblers(NULL, NULL);
  maple_attach_callback(MAPLE_FUNC_PURUPURU, scan_local_rumblers, NULL);
  maple_detach_callback(MAPLE_FUNC_PURUPURU, scan_local_rumblers, NULL);
}

size_t enj_rumble_states_length(void) { return MAPLE_PORT_COUNT; }
void enj_rumble_rate_limit_set(int frames) {
  enj_rumble_rate_limit = frames < 0 ? 0 : frames;
}

enj_rumble_reply_e enj_rumble_effect_set_raw(enj_ctrl_port_name_e ctrloffset,
                                             uint32_t raw) {
  if ((unsigned)ctrloffset >= MAPLE_PORT_COUNT) {
    return ENJ_RUMBLE_NO_DEVICE;
  }
  maple_device_t *rumble_dev = local_rumbles[ctrloffset];
  if (rumble_dev == NULL) {
    return ENJ_RUMBLE_NO_DEVICE;
  }
  enj_rumble_reply_e reply = ENJ_RUMBLE_SET;
  if (pending_rumble_effects[ctrloffset] != 0) {
    reply |= ENJ_RUMBLE_OVERWROTE_PREVIOUS;
  }
  if (rumble_rate_limits[ctrloffset] > 0) {
    reply |= ENJ_RUMBLE_RATE_LIMITED;
  }
  pending_rumble_effects[ctrloffset] = raw;
  return reply;
}

enj_rumble_reply_e enj_rumble_effect_set(enj_ctrl_port_name_e ctrloffset,
                                         purupuru_effect_t effect) {
  return enj_rumble_effect_set_raw(ctrloffset, effect.raw);
}

maple_device_t **enj_rumble_states_get(void) { return local_rumbles; }

void enj_rumble_update(void) {
  for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
    if (rumble_rate_limits[i] > 0) {
      rumble_rate_limits[i]--;
    }
    if (local_rumbles[i] == NULL) {
      pending_rumble_effects[i] = 0;
      rumble_rate_limits[i] = 0;
      continue;
    }
    if (pending_rumble_effects[i] != 0 && rumble_rate_limits[i] == 0) {
      purupuru_rumble_raw(local_rumbles[i], pending_rumble_effects[i]);
      pending_rumble_effects[i] = 0;
      rumble_rate_limits[i] = enj_rumble_rate_limit;
    }
  }
}
