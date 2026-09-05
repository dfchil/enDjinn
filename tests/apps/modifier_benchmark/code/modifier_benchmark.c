#include <enDjinn/enj_enDjinn.h>

#include <stdlib.h>

static int modifier_triangle_count;

static void render_receiver(void *__unused data) {
  pvr_poly_cxt_t context;
  pvr_poly_cxt_col_mod(&context, PVR_LIST_TR_POLY);
  context.gen.culling = PVR_CULLING_NONE;
  context.depth.comparison = PVR_DEPTHCMP_ALWAYS;
  context.depth.write = 0;
  pvr_poly_mod_hdr_t *header = (pvr_poly_mod_hdr_t *)pvr_dr_target();
  pvr_poly_mod_compile(header, &context);
  pvr_dr_commit(header);

  static const float positions[4][2] = {
      {0.0f, 480.0f}, {0.0f, 0.0f}, {640.0f, 480.0f}, {640.0f, 0.0f}};
  for (int i = 0; i < 4; i++) {
    pvr_vertex_pcm_t *vertex = (pvr_vertex_pcm_t *)pvr_dr_target();
    vertex->flags = i == 3 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
    vertex->x = positions[i][0];
    vertex->y = positions[i][1];
    vertex->z = 1.0f;
    vertex->argb0 = 0x90004080u;
    vertex->argb1 = 0x90804020u;
    pvr_dr_commit(vertex);
  }
}

static void submit_modifier_triangle(int index) {
  pvr_mod_hdr_t *header = (pvr_mod_hdr_t *)pvr_dr_target();
  const uint32_t mode = index == modifier_triangle_count - 1
                            ? PVR_MODIFIER_INCLUDE_LAST_POLY
                            : PVR_MODIFIER_OTHER_POLY;
  pvr_mod_compile(header, PVR_LIST_TR_MOD, mode, PVR_CULLING_NONE);
  pvr_dr_commit(header);

  pvr_modifier_vol_t *first;
  pvr_modifier_vol_t *second;
  const float x = (float)((index * 37) % 620);
  const float y = (float)((index * 53) % 460);
  const float z = 1.25f + (float)(index % 17) * 0.05f;
  enj_draw_pvr_dr64_init((void **)&first, (void **)&second);
  first->flags = PVR_CMD_VERTEX_EOL;
  first->ax = x;
  first->ay = y;
  first->az = z;
  first->bx = x + 20.0f;
  first->by = y;
  first->bz = z;
  first->cx = (index & 1) == 0 ? x : x + 20.0f;
  enj_draw_pvr_dr64_commit_1st();
  second->cy = y + 20.0f;
  second->cz = z;
  enj_draw_pvr_dr64_commit_2nd();
}

static void render_modifiers(void *__unused data) {
  for (int i = 0; i < modifier_triangle_count; i++) {
    submit_modifier_triangle(i);
  }
}

static void update(void *__unused data) {
  enj_render_list_add(PVR_LIST_TR_POLY, render_receiver, NULL);
  if (modifier_triangle_count > 0) {
    enj_render_list_add(PVR_LIST_TR_MOD, render_modifiers, NULL);
  }
}

int main(__unused int argc, __unused char **argv) {
  const char *count = getenv("ENJ_MODIFIER_BENCH_TRIANGLES");
  modifier_triangle_count = count != NULL ? atoi(count) : 0;
  if (modifier_triangle_count < 0) return -1;

  enj_state_init_defaults();
  if (enj_state_startup() != 0) return -1;
  enj_mode_t mode = {.name = "Modifier benchmark", .mode_updater = update};
  enj_mode_push(&mode);
  enj_state_run();
  return 0;
}
