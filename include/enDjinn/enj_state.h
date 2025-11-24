#ifndef ENJ_STATE_H
#define ENJ_STATE_H

#include <enDjinn/enj_ctrlr.h>

typedef struct enj_state_s {
    struct {
        uint32_t initialized : 1;
        uint32_t end_of_sequence : 1;
        uint32_t shut_down : 1;
        uint32_t : 30;
    } flags;
    uint32_t exit_pattern;
} enj_state_t;


enj_state_t* enj_get_state(void);
void enj_state_init(void);

#endif  // ENJ_STATE_H

