#include <enDjinn/enj_enDjinn.h>

static const alignas(32) uint8_t texture32_raw[] = {
#embed "../embeds/example/texture/pal8/enDjinn512.dt"
    // #embed "../build/pvrtex/rgb565_vq_tw/sh4zam32.dt"
};
static const alignas(32) uint8_t palette32_raw[] = {
#embed "../embeds/example/texture/pal8/enDjinn512.dt.pal"
};


void only_mode_updater(void *data) {
  uint32_t *counter = (uint32_t *)data;
  (*counter)++;
  printf("Main Mode Counter: %u\n", *counter);
}

int main(__unused int argc, __unused char **argv) {

  enj_state_defaults();

  // default pattern is START + A + B + X + Y held down
  // lets make it easier
  // the reason for using the enj_ctrlr_state_t interface
  // for making the bit-pattern is that each button is
  // mapped to 2 bits for tracking position and if a change was this frame
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

  //* setup at least one mode */

  uint32_t counter = 0;

  enj_game_mode_t main_mode = {
      .name = "Main Mode",
      .mode_updater = only_mode_updater,
      .data = &counter,
  };

  enj_mode_push(&main_mode);

  enj_run();
}
