#ifndef ENJ_RUMBLE_H
#define ENJ_RUMBLE_H

#include <enDjinn/enj_api.h>
#include <enDjinn/enj_ctrl.h>

ENJ_BEGIN_DECLS

typedef enum {
  ENJ_RUMBLE_SET = 1,
  ENJ_RUMBLE_NO_DEVICE = 2,
  ENJ_RUMBLE_RATE_LIMITED = 4,
  ENJ_RUMBLE_OVERWROTE_PREVIOUS = 8,
  ENJ_RUMBLE_UNSPECIFIED_ERROR = 16,

  /* Source-compatible aliases for the original pre-convention names. */
  enj_rumble_set = ENJ_RUMBLE_SET,
  enj_rumble_no_device = ENJ_RUMBLE_NO_DEVICE,
  enj_rumble_rate_limited = ENJ_RUMBLE_RATE_LIMITED,
  enj_rumble_overwrote_previous = ENJ_RUMBLE_OVERWROTE_PREVIOUS,
  enj_rumble_unspecified_error = ENJ_RUMBLE_UNSPECIFIED_ERROR,
} enj_rumble_reply_e;

/** Set the minimum number of frames between commands. Negative values become 0. */
void enj_rumble_rate_limit_set(int frames);

/**
 * Set a rumble effect on a controller port
 * @param ctrloffset Controller port to set the rumble effect on
 * @param raw Raw purupuru_effect_t structure as a uint32_t
 * @return enj_rumble_reply_e value indicating success or failure and any
 * relevant flags
 */
enj_rumble_reply_e enj_rumble_effect_set_raw(enj_ctrl_port_name_e ctrloffset, uint32_t raw);

/**
 * Set a rumble effect on a controller port
 * @param ctrloffset Controller port to set the rumble effect on
 * @param effect purupuru_effect_t structure defining the rumble effect
 * @return enj_rumble_reply_e value indicating success or failure and any
 * relevant flags
 * 
 * @note This is a convenience wrapper around enj_rumble_effect_set_raw
 */
enj_rumble_reply_e enj_rumble_effect_set(enj_ctrl_port_name_e ctrloffset, purupuru_effect_t effect);

/**
 * Get the length of the rumble device states array
 * @return Length of the rumble device states array
 * 
 * @note Currently always MAPLE_PORT_COUNT
 */
size_t enj_rumble_states_length(void);

/**
 * Get the current rumble device states
 * @return Array of maple_device_t pointers for each rumble device
 * @note A NULL pointer indicates no rumble device is connected on that port.
 */
maple_device_t **enj_rumble_states_get(void);

ENJ_END_DECLS

#endif // ENJ_RUMBLE_H
