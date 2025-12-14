#ifndef ENJ_RUMBLE_H
#define ENJ_RUMBLE_H

#include <enDjinn/enj_ctrl.h>

typedef enum uint8_t {
  enj_rumble_set = 1,
  enj_rumble_no_device = 2,
  enj_rumble_rate_limited = 4,
  enj_rumble_overwrote_previous = 8,
  enj_rumble_unspecified_error = 16,
} enj_rumbler_reply_e;

void enj_rumble_set_rate_limit(int frames);

enj_rumbler_reply_e enj_rumbler_set(enj_ctrl_port_name_e ctrloffset, uint32_t raw);



#endif // ENJ_RUMBLE_H
