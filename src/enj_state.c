#include <enDjinn/enj_enDjinn.h>

// static const uint32_t exit_pattern = ;

alignas(32) static enj_state_t state = { 0 };
enj_state_t* enj_get_state(void) { return &state; }


void enj_state_init(void) {
    state.flags.initialized = 1;
    state.exit_pattern = ((enj_ctrlr_state_t){.buttons = {
                             .START = BUTTON_DOWN,
                             //.A = BUTTON_DOWN,
                            //  .B = BUTTON_DOWN,
                              .X = BUTTON_DOWN,
                             //  .Y = BUTTON_DOWN
                         }}).buttons.raw;
}