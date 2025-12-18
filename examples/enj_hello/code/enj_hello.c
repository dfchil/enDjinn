#include <enDjinn/enj_enDjinn.h>
#define MARGIN_LEFT (20 * ENJ_XSCALE)

void render_PT(void *__unused) {
  enj_font_scale_set(4);
  enj_qfont_write("Hello, enDjinn!", MARGIN_LEFT, 20, PVR_LIST_PT_POLY);
  enj_font_scale_set(1);
  enj_qfont_write("Press START+A+B+X+Y to end program.", MARGIN_LEFT, 120,
                  PVR_LIST_PT_POLY);
}
void main_mode_updater(void *__unused) {
  enj_render_list_add(PVR_LIST_PT_POLY, render_PT, NULL);
}
int main(__unused int argc, __unused char **argv) {
  // initialize enDjinn state with default values
  enj_state_init_defaults();
  if (enj_state_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }
  enj_mode_t main_mode = {
      .name = "Main Mode",
      .mode_updater = main_mode_updater,
      .data = NULL,
  };
  enj_mode_push(&main_mode);
  enj_state_run();
  return 0;
}
