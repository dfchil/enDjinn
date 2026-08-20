#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>
#include <sh4zam/shz_sh4zam.h>

#define MARGIN_LEFT (20 * ENJ_XSCALE)

static int testlist = PVR_LIST_OP_POLY;
static int p_x_off = -1; // polygon offset X
static int p_y_off = -1; // polygon offset Y
static int v_x_off = -1; // modifier volume offset X
static int v_y_off = -1; // modifier volume offset Y

void render_modifiable(void *__unused) {
  pvr_dr_state_t dr_state;
  pvr_dr_init(&dr_state);

  pvr_poly_cxt_t cxt;
  pvr_poly_cxt_col_mod(&cxt, testlist);
  cxt.gen.culling = PVR_CULLING_NONE;
  pvr_poly_mod_hdr_t *hdr = (pvr_poly_mod_hdr_t *)pvr_dr_target(dr_state);
  pvr_poly_mod_compile(hdr, &cxt);
  pvr_dr_commit(hdr);

  shz_vec3_t verts[4] = {
      {.x = 0.0f, .y = 240.0f, .z = 5.5f},
      {.x = 0.0f, .y = 0.0f, .z = 5.5f},
      {.x = 320.0f, .y = 240.0f, .z = 5.5f},
      {.x = 320.0f, .y = 0.0f, .z = 5.5f},
  };

  for (int i = 0; i < 4; i++) {
    pvr_vertex_pcm_t *vert = (pvr_vertex_pcm_t *)pvr_dr_target(dr_state);
    vert->flags = i == 3 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
    vert->x = verts[i].x + p_x_off;
    vert->y = verts[i].y + p_y_off;
    vert->z = verts[i].z;
    vert->argb0 = 0x7f0000ff; // Blue outside
    vert->argb1 = 0x7f00ff00; // Green inside
    pvr_dr_commit(vert);
  }
  pvr_dr_finish();
}

void render_modifier(void *__unused) {
  pvr_dr_state_t dr_state;
  pvr_dr_init(&dr_state);

  shz_vec3_t verts[4] = {
      {.x = 80.0f, .y = 60.0f, .z = 6.6f},
      {.x = 240.0f, .y = 60.0f, .z = 6.6f},
      {.x = 240.0f, .y = 180.0f, .z = 6.6f},
      {.x = 80.0f, .y = 180.0f, .z = 6.6f},
  };
  unsigned char indexes[2][3] = {
      {0, 1, 2},
      {0, 2, 3},
  };
  for (int i = 0; i < 2; i++) {
    pvr_mod_hdr_t *hdr = (pvr_mod_hdr_t *)pvr_dr_target(dr_state);
    pvr_mod_compile(hdr, testlist + 1, PVR_MODIFIER_INCLUDE_LAST_POLY,
                    PVR_CULLING_NONE);
    pvr_dr_commit(hdr);

    pvr_modifier_vol_t *modvol = (pvr_modifier_vol_t *)pvr_dr_target(dr_state);
    modvol->flags = PVR_CMD_VERTEX_EOL;
    modvol->ax = verts[indexes[i][0]].x + v_x_off;
    modvol->ay = verts[indexes[i][0]].y + v_y_off;
    modvol->az = verts[indexes[i][0]].z;
    modvol->bx = verts[indexes[i][1]].x + v_x_off;
    modvol->by = verts[indexes[i][1]].y + v_y_off;
    modvol->bz = verts[indexes[i][1]].z;
    modvol->cx = verts[indexes[i][2]].x + v_x_off;
    pvr_dr_commit(modvol);
    modvol = (pvr_modifier_vol_t *)pvr_dr_target(dr_state);
    pvr_modifier_vol_t *modvol_p2 = (pvr_modifier_vol_t *)((int)modvol - 32);
    modvol_p2->cy = verts[indexes[i][2]].y + v_y_off;
    modvol_p2->cz = verts[indexes[i][2]].z;
    pvr_dr_commit(modvol);
  }
  pvr_dr_finish();
}

void render_PT(void *__unused) {
  enj_font_scale_set(4);
  enj_qfont_write("Modifier example", MARGIN_LEFT, 20,
                  PVR_LIST_PT_POLY);
  enj_font_scale_set(1);

  enj_qfont_write("Press A to toggle between OP and TR lists", MARGIN_LEFT, 120,
                  PVR_LIST_PT_POLY);
  enj_qfont_write("(TR breaks rendering)", MARGIN_LEFT, 140, PVR_LIST_PT_POLY);
  enj_qfont_write("Press START to end program.", MARGIN_LEFT, 160,
                  PVR_LIST_PT_POLY);

  enj_font_zvalue_set(5.0f);
  enj_qfont_write("Occluded text", MARGIN_LEFT * 2, 200, PVR_LIST_PT_POLY);
  enj_font_zvalue_set(10.0f);
}
void main_mode_updater(void *__unused) {
  enj_ctrlr_state_t **ctrls = enj_ctrl_get_states();
  for (int i = 0; i < 4; i++) {
    if (ctrls[i] != NULL) {
      if (ctrls[i]->button.A == ENJ_BUTTON_DOWN_THIS_FRAME) {
        testlist =
            testlist == PVR_LIST_OP_POLY ? PVR_LIST_TR_POLY : PVR_LIST_OP_POLY;
      }
      if (ctrls[i]->button.UP == ENJ_BUTTON_DOWN) {
        p_y_off -= 5;
      }
      if (ctrls[i]->button.DOWN == ENJ_BUTTON_DOWN) {
        p_y_off += 5;
      }
      if (ctrls[i]->button.LEFT == ENJ_BUTTON_DOWN) {
        p_x_off -= 5;
      }
      if (ctrls[i]->button.RIGHT == ENJ_BUTTON_DOWN) {
        p_x_off += 5;
      }
    }
  }
  enj_render_list_add(PVR_LIST_PT_POLY, render_PT, NULL);
  enj_render_list_add(testlist, render_modifiable, NULL);
  enj_render_list_add(testlist + 1, render_modifier, NULL);
}

int main(__unused int argc, __unused char **argv) {
  // initialize enDjinn state with default values
  enj_state_init_defaults();
  enj_state_soft_reset_set(ENJ_BUTTON_DOWN << (8 << 1));
  if (enj_state_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }
  p_x_off = (vid_mode->width >> 1) - 160;
  p_y_off = (vid_mode->height >> 1) - 120;
  v_x_off = (vid_mode->width >> 1) - 160;
  v_y_off = (vid_mode->height >> 1) - 120;

  enj_mode_t main_mode = {
      .name = "Main Mode",
      .mode_updater = main_mode_updater,
      .data = NULL,
  };
  enj_mode_push(&main_mode);
  enj_state_run();
  return 0;
}
