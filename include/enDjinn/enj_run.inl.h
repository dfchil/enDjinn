#include <enDjinn/enj_run.h>

// void enj_run(void) {
//   while (1) {
//     enj_dc_ctrlrs_map_state();
//     if (check_c1_exit()) {
//       break;
//     }
//     // if (enj_mode_get_index() != title_screen_stack_index) {
//     //   enj_ctrlr_state_t **cstates = enj_get_ctrlr_states();
//     //   for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
//     //     if (detect_shutdown_combo(cstates[i])) {
//     //       cstates[i]->START = BUTTON_DOWN;
//     //       cstates[i]->BTN_A = BUTTON_DOWN;
//     //       cut_to_title_screen();
//     //     }
//     //   }
//     // }

//     // if (flags.end_of_sequence) {
//     //   enj_mode_pop();
//     //   enj_mode_push(get_zoom_transition_mode());

//     //   flags.end_of_sequence = 0;
//     // }
//     enj_next_frame(enj_mode_get());
//   }
// }