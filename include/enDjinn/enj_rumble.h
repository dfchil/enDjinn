#ifndef ENJ_RUMBLE_H
#define ENJ_RUMBLE_H

#include <enDjinn/enj_ctrl.h>
#include <dc/maple/purupuru.h>

typedef enum uint8_t {
  enj_rumble_set = 1,
  enj_rumble_no_device = 2,
  enj_rumble_rate_limited = 4,
  enj_rumble_overwrote_previous = 8,
  enj_rumble_unspecified_error = 16,
} enj_rumble_reply_e;

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
maple_device_t** enj_rumble_states_get(void);

#endif // ENJ_RUMBLE_H
