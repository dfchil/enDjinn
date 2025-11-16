#ifndef GAME_CONTROLLER_DREAMCAST_H
#define GAME_CONTROLLER_DREAMCAST_H

#include <enDJinn/controller/abstract.h>

int dc_ctrlrs_map_state(void);
maple_device_t *maple_port_type(int p, uint32 func);
controller_state_t **get_ctrlr_states(void);

#include "dreamcast.inl.h"

#endif // GAME_CONTROLLER_DREAMCAST_H