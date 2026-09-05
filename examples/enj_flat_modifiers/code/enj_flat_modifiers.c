#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>

#include <stdint.h>

#define LOGICAL_WIDTH 320.0f
#define LOGICAL_HEIGHT 240.0f
#define MARGIN_LEFT (20 * ENJ_XSCALE)

typedef struct point3 {
  float x;
  float y;
  float z;
} point3_t;

static pvr_list_t receiver_list = PVR_LIST_OP_POLY;
static float x_offset;
static float y_offset;

static void render_receiver(void *__unused data) {
  pvr_poly_cxt_t context;
  pvr_poly_cxt_col_mod(&context, receiver_list);
  context.gen.culling = PVR_CULLING_NONE;
  pvr_poly_mod_hdr_t *header = (pvr_poly_mod_hdr_t *)pvr_dr_target();
  pvr_poly_mod_compile(header, &context);
  pvr_dr_commit(header);

  static const point3_t corners[4] = {
      {0.0f, 240.0f, 5.5f},
      {0.0f, 0.0f, 5.5f},
      {320.0f, 240.0f, 5.5f},
      {320.0f, 0.0f, 5.5f},
  };
  for (int i = 0; i < 4; i++) {
    pvr_vertex_pcm_t *vertex = (pvr_vertex_pcm_t *)pvr_dr_target();
    vertex->flags = i == 3 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
    vertex->x = corners[i].x + x_offset;
    vertex->y = corners[i].y + y_offset;
    vertex->z = corners[i].z;
    vertex->argb0 = 0xff2458b8u;
    vertex->argb1 = 0xff35b86bu;
    pvr_dr_commit(vertex);
  }
}

static void submit_modifier_triangle(const point3_t *a, const point3_t *b,
                                     const point3_t *c, uint32_t mode) {
  pvr_mod_hdr_t *header = (pvr_mod_hdr_t *)pvr_dr_target();
  pvr_mod_compile(header, receiver_list + 1, mode, PVR_CULLING_NONE);
  pvr_dr_commit(header);

  pvr_modifier_vol_t *first;
  pvr_modifier_vol_t *second;
  enj_draw_pvr_dr64_init((void **)&first, (void **)&second);
  first->flags = PVR_CMD_VERTEX_EOL;
  first->ax = a->x + x_offset;
  first->ay = a->y + y_offset;
  first->az = a->z;
  first->bx = b->x + x_offset;
  first->by = b->y + y_offset;
  first->bz = b->z;
  first->cx = c->x + x_offset;
  enj_draw_pvr_dr64_commit_1st();
  second->cy = c->y + y_offset;
  second->cz = c->z;
  enj_draw_pvr_dr64_commit_2nd();
}

static void render_modifier(void *__unused data) {
  static const point3_t corners[4] = {
      {80.0f, 60.0f, 6.6f},
      {240.0f, 60.0f, 6.6f},
      {240.0f, 180.0f, 6.6f},
      {80.0f, 180.0f, 6.6f},
  };
  submit_modifier_triangle(&corners[0], &corners[1], &corners[2],
                           PVR_MODIFIER_OTHER_POLY);
  submit_modifier_triangle(&corners[0], &corners[2], &corners[3],
                           PVR_MODIFIER_INCLUDE_LAST_POLY);
}

static void render_labels(void *__unused data) {
  enj_font_scale_set(4);
  enj_qfont_write("Flat modifiers", MARGIN_LEFT, 20, PVR_LIST_PT_POLY);
  enj_font_scale_set(1);
  enj_qfont_write("A: toggle opaque / transparent receiver", MARGIN_LEFT, 140,
                  PVR_LIST_PT_POLY);
  enj_qfont_write("D-pad: move the planar receiver and mask", MARGIN_LEFT, 160,
                  PVR_LIST_PT_POLY);
}

static void main_mode_updater(void *__unused data) {
  enj_ctrlr_state_t **controllers = enj_ctrl_get_states();
  for (int i = 0; i < 4; i++) {
    if (controllers[i] == NULL) {
      continue;
    }
    if (controllers[i]->button.A == ENJ_BUTTON_DOWN_THIS_FRAME) {
      receiver_list = receiver_list == PVR_LIST_OP_POLY
                          ? PVR_LIST_TR_POLY
                          : PVR_LIST_OP_POLY;
    }
    if (controllers[i]->button.UP == ENJ_BUTTON_DOWN) y_offset -= 5.0f;
    if (controllers[i]->button.DOWN == ENJ_BUTTON_DOWN) y_offset += 5.0f;
    if (controllers[i]->button.LEFT == ENJ_BUTTON_DOWN) x_offset -= 5.0f;
    if (controllers[i]->button.RIGHT == ENJ_BUTTON_DOWN) x_offset += 5.0f;
  }
  enj_render_list_add(receiver_list, render_receiver, NULL);
  enj_render_list_add(receiver_list + 1, render_modifier, NULL);
  enj_render_list_add(PVR_LIST_PT_POLY, render_labels, NULL);
}

int main(__unused int argc, __unused char **argv) {
  enj_state_init_defaults();
  enj_state_soft_reset_set(ENJ_BUTTON_DOWN << (8 << 1));
  if (enj_state_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }

  x_offset = (float)(vid_mode->width >> 1) - LOGICAL_WIDTH * 0.5f;
  y_offset = (float)(vid_mode->height >> 1) - LOGICAL_HEIGHT * 0.5f;
  enj_mode_t mode = {
      .name = "Flat Modifiers",
      .mode_updater = main_mode_updater,
      .data = NULL,
  };
  enj_mode_push(&mode);
  enj_state_run();
  return 0;
}
