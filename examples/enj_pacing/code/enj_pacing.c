#include <enDjinn/enj_enDjinn.h>
#include <stdio.h>

/* A bar moves one step per tick: smooth when every tick is shown, a visible
 * jump where a scene was dropped, and slower where the game slows down. Up and
 * Down add or remove full-screen translucent layers until the PVR runs late;
 * A switches the pacing. */

#define MARGIN_LEFT (20 * ENJ_XSCALE)
#define MAX_LAYERS 64

typedef struct {
  pvr_sprite_hdr_t layer_hdr;
  pvr_sprite_hdr_t bar_hdr;
  int layers;
  float bar_x;
} pacing_data_t;

static const char *const mode_names[] = {"FREE", "STRICT", "DROP_LATE", "HALF"};

static void quad(pvr_sprite_hdr_t *hdr, float x0, float y0, float x1, float y1,
                 float z) {
  float corners[4][3] = {{x0, y0, z}, {x1, y0, z}, {x1, y1, z}, {x0, y1, z}};
  enj_draw_sprite(corners, hdr, NULL);
}

static void render_tr(void *data) {
  pacing_data_t *pd = data;
  const float w = 640.0f * ENJ_XSCALE;
  for (int i = 0; i < pd->layers; i++) {
    quad(&pd->layer_hdr, 0.0f, 0.0f, w, 480.0f, 1.0f + (float)i);
  }
  quad(&pd->bar_hdr, pd->bar_x * ENJ_XSCALE, 200.0f,
       (pd->bar_x + 24.0f) * ENJ_XSCALE, 280.0f, 1.0f + MAX_LAYERS);
}

static void render_pt(void *data) {
  pacing_data_t *pd = data;
  enj_pacing_stats_t st;
  enj_pacing_stats(&st);
  char line[96];
  snprintf(line, sizeof(line), "pacing %s (A)   layers %d (Up/Down)",
           mode_names[enj_pacing_get()], pd->layers);
  enj_qfont_write(line, MARGIN_LEFT, 20, PVR_LIST_PT_POLY);
  snprintf(line, sizeof(line), "ticks %lu  rendered %lu  dropped %lu  late %lu",
           (unsigned long)st.ticks, (unsigned long)st.rendered,
           (unsigned long)st.dropped, (unsigned long)st.late);
  enj_qfont_write(line, MARGIN_LEFT, 40, PVR_LIST_PT_POLY);
}

static void pacing_updater(void *data) {
  pacing_data_t *pd = data;
  enj_ctrlr_state_t **ctrls = enj_ctrl_get_states();
  for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
    if (ctrls[i] == NULL) {
      continue;
    }
    if (ctrls[i]->button.A == ENJ_BUTTON_DOWN_THIS_FRAME) {
      enj_pacing_set((enj_pacing_t)((enj_pacing_get() + 1) % 4));
    }
    if (ctrls[i]->button.UP == ENJ_BUTTON_DOWN_THIS_FRAME &&
        pd->layers < MAX_LAYERS) {
      pd->layers++;
    }
    if (ctrls[i]->button.DOWN == ENJ_BUTTON_DOWN_THIS_FRAME &&
        pd->layers > 0) {
      pd->layers--;
    }
  }
  // Game state advances on every tick, shown or not.
  pd->bar_x += 4.0f;
  if (pd->bar_x > 616.0f) {
    pd->bar_x = 0.0f;
  }
  // Nothing to build for a scene that will not be shown.
  if (enj_pacing_scene_dropped()) {
    return;
  }
  enj_render_list_add(PVR_LIST_TR_POLY, render_tr, pd);
  enj_render_list_add(PVR_LIST_PT_POLY, render_pt, pd);
}

int main(__unused int argc, __unused char **argv) {
  enj_state_init_defaults();
  if (enj_state_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }
  static pacing_data_t pd = {.layers = 0, .bar_x = 0.0f};
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_col(&cxt, PVR_LIST_TR_POLY);
  pvr_sprite_compile(&pd.layer_hdr, &cxt);
  pd.layer_hdr.argb = 0x10406080;
  pvr_sprite_compile(&pd.bar_hdr, &cxt);
  pd.bar_hdr.argb = 0xffffd040;

  enj_pacing_set(ENJ_PACING_DROP_LATE);
  enj_mode_t main_mode = {
      .name = "Pacing",
      .mode_updater = pacing_updater,
      .data = &pd,
  };
  enj_mode_push(&main_mode);
  enj_state_run();
  return 0;
}
