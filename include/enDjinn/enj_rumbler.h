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
} enj_rumbler_reply_e;

void enj_rumble_set_rate_limit(int frames);

enj_rumbler_reply_e enj_rumbler_set_effect(enj_ctrl_port_name_e ctrloffset, uint32_t raw);

/**
 * Get the current rumble device states
 * @return Array of maple_device_t pointers for each rumble device
 * @note The array has MAPLE_PORT_COUNT elements, corresponding to each port.
 * @note A NULL pointer indicates no rumble device is connected on that port.
 */
maple_device_t** enj_rumbler_get_states();

#endif // ENJ_RUMBLE_H
