#include <enDjinn/enj_enDjinn.h>

int main(__unused int argc, __unused char **argv) {

  enj_state_defaults();
  enj_state_set_exit_pattern((enj_ctrlr_state_t){
      .buttons =
          {
              .START = BUTTON_DOWN,
              .A = BUTTON_DOWN,
          },
  }.buttons.raw);
  if (enj_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }
  enj_run();
}
