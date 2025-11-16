#ifndef ENDJINN_CONTROLLER_AICONTROLLER_H
#define ENDJINN_CONTROLLER_AICONTROLLER_H

#include <enDJinn/controller/abstract_ctrlr.h>
#include <enDJinn/controller/core.h>

void ai_set_next_goal(abstract_controller_t *controller);

void read_aibot_controller(abstract_controller_t *controller,
                           controller_state_t *ctrlr);

#endif // ENDJINN_CONTROLLER_AICONTROLLER_H
