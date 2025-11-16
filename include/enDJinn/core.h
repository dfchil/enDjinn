#ifndef ENDJINN_CORE_H
#define ENDJINN_CORE_H


#include <enDJinn/controller/abstract.h>

int detect_shutdown_combo(controller_state_t *cstate);

void core_flag_endofsequence(void);
void core_flag_shutdown(void);

void core_loop(void);
int core_init(void);
void cut_to_title_screen(void);

void heapUtilization(void);

#include "core.inl.h"

#endif // ENDJINN_CORE_H
