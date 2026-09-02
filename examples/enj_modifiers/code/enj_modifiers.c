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
  pvr_poly_cxt_t cxt;
  pvr_poly_cxt_col_mod(&cxt, testlist);
  cxt.gen.culling = PVR_CULLING_CCW;
  pvr_poly_mod_hdr_t *hdr = (pvr_poly_mod_hdr_t *)pvr_dr_target();
  pvr_poly_mod_compile(hdr, &cxt);
  pvr_dr_commit(hdr);

  const float depths[3] = {0.5f, 2.0f, 3.5f};
  for (int band = 0; band < 3; band++) {
    const float left = (320.0f * band) / 3.0f;
    const float right = (320.0f * (band + 1)) / 3.0f;
    const shz_vec3_t verts[4] = {
        {.x = left, .y = 240.0f, .z = depths[band]},
        {.x = left, .y = 0.0f, .z = depths[band]},
        {.x = right, .y = 240.0f, .z = depths[band]},
        {.x = right, .y = 0.0f, .z = depths[band]},
    };
    for (int i = 0; i < 4; i++) {
      pvr_vertex_pcm_t *vert = (pvr_vertex_pcm_t *)pvr_dr_target();
      vert->flags = i == 3 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
      vert->x = verts[i].x + p_x_off;
      vert->y = verts[i].y + p_y_off;
      vert->z = verts[i].z;
      vert->argb0 = 0xff0000ff; // Blue outside
      vert->argb1 = 0xff00ff00; // Green inside
      pvr_dr_commit(vert);
    }
  }
}

static void render_depth_slab(float left, float top, float right, float bottom,
                              uint32_t final_mode) {
  const shz_vec3_t verts[8] = {
      {.x = left, .y = top, .z = 3.0f},
      {.x = right, .y = top, .z = 3.0f},
      {.x = right, .y = bottom, .z = 3.0f},
      {.x = left, .y = bottom, .z = 3.0f},
      {.x = left, .y = top, .z = 1.0f},
      {.x = right, .y = top, .z = 1.0f},
      {.x = right, .y = bottom, .z = 1.0f},
      {.x = left, .y = bottom, .z = 1.0f},
  };
  const unsigned char indexes[4][3] = {
      {0, 1, 2}, {0, 2, 3}, {4, 6, 5}, {4, 7, 6},
  };
  for (int i = 0; i < 4; i++) {
    pvr_mod_hdr_t *hdr = (pvr_mod_hdr_t *)pvr_dr_target();
    pvr_mod_compile(hdr, testlist + 1,
                    i == 3 ? final_mode : PVR_MODIFIER_OTHER_POLY,
                    PVR_CULLING_NONE);
    pvr_dr_commit(hdr);
    pvr_modifier_vol_t *modvol_p1, *modvol_p2;
    enj_draw_pvr_dr64_init((void **)&modvol_p1, (void **)&modvol_p2);
    modvol_p1->flags = PVR_CMD_VERTEX_EOL;
    modvol_p1->ax = verts[indexes[i][0]].x + v_x_off;
    modvol_p1->ay = verts[indexes[i][0]].y + v_y_off;
    modvol_p1->az = verts[indexes[i][0]].z;
    modvol_p1->bx = verts[indexes[i][1]].x + v_x_off;
    modvol_p1->by = verts[indexes[i][1]].y + v_y_off;
    modvol_p1->bz = verts[indexes[i][1]].z;
    modvol_p1->cx = verts[indexes[i][2]].x + v_x_off;
    enj_draw_pvr_dr64_commit_1st();
    modvol_p2->cy = verts[indexes[i][2]].y + v_y_off;
    modvol_p2->cz = verts[indexes[i][2]].z;
    enj_draw_pvr_dr64_commit_2nd();
  }
}

void render_modifier(void *__unused) {
  render_depth_slab(40.0f, 60.0f, 280.0f, 180.0f,
                    PVR_MODIFIER_INCLUDE_LAST_POLY);
  render_depth_slab(130.0f, 90.0f, 190.0f, 150.0f,
                    PVR_MODIFIER_EXCLUDE_LAST_POLY);
}

void render_PT(void *__unused) {
  enj_font_scale_set(4);
  enj_qfont_write("Modifier example", MARGIN_LEFT, 20, PVR_LIST_PT_POLY);
  enj_font_scale_set(1);

  enj_qfont_write("Press A to toggle between OP and TR lists", MARGIN_LEFT, 120,
                  PVR_LIST_PT_POLY);
  enj_qfont_write("OP: only the middle-depth band turns green", MARGIN_LEFT,
                  140, PVR_LIST_PT_POLY);
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
