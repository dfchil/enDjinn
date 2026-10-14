/*
 * Closed modifier-volume and near-plane clipping example.
 *
 * The cube topology and test scenario are adapted from KallistiOS's
 * examples/dreamcast/pvr/modifier_volume_zclip example (Twada, 2024).
 * This version is self-contained and submits through enDjinn's render-list
 * callbacks so the same source runs on Dreamcast and pc-enDjinn.
 */
#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>

#include <math.h>
#include <stdint.h>
#ifdef ENJ_DEEP_MODIFIERS_TEST_CONTROLS
#include <stdlib.h>
#endif

#define LOGICAL_WIDTH 320.0f
#define LOGICAL_HEIGHT 240.0f
#define MAX_CLIPPED_TRIANGLES 48
#define MAX_CAP_VERTICES 24
#define CAMERA_NEAR 1.25f
#define CAMERA_FOCAL 150.0f
#define BOX_FLOOR_EPSILON 0.01f

typedef struct vec3 {
  float x;
  float y;
  float z;
} vec3_t;

typedef struct triangle {
  vec3_t v[3];
} triangle_t;

typedef struct cap_vertex {
  vec3_t p;
  float angle;
} cap_vertex_t;

typedef enum receiver_mode {
  RECEIVER_OPAQUE,
  RECEIVER_TRANSLUCENT,
  RECEIVER_MODIFIER_TRANSLUCENT,
  RECEIVER_MODE_COUNT,
} receiver_mode_t;

static float screen_x_offset;
static float screen_y_offset;
static vec3_t cube_angle = {0.25f, 0.55f, 0.0f};
static float box_angle = 0.32f;
static receiver_mode_t receiver_mode = RECEIVER_OPAQUE;
static pvr_list_t receiver_list = PVR_LIST_OP_POLY;
static int show_modifier_cube = 1;
static int modifier_effect_enabled = 1;

static uint32_t receiver_alpha(int modifier_area) {
  if (receiver_mode == RECEIVER_OPAQUE) {
    return 0xff000000u;
  }
  if (receiver_mode == RECEIVER_MODIFIER_TRANSLUCENT) {
    return modifier_area ? 0x70000000u : 0xff000000u;
  }
  return 0x90000000u;
}

static vec3_t project_world(vec3_t world) {
  /* KOS example's default camera: eye=(0,2,5), target=(0,0,0). */
  const float forward_y = -0.37139067f;
  const float forward_z = -0.92847669f;
  const float up_y = 0.92847669f;
  const float up_z = -0.37139067f;
  const float relative_y = world.y - 2.0f;
  const float relative_z = world.z - 5.0f;
  const float depth = relative_y * forward_y + relative_z * forward_z;
  const float camera_y = relative_y * up_y + relative_z * up_z;
  return (vec3_t){
      .x = screen_x_offset + 160.0f + CAMERA_FOCAL * world.x / depth,
      .y = screen_y_offset + 112.0f - CAMERA_FOCAL * camera_y / depth,
      .z = CAMERA_NEAR / depth,
  };
}

static vec3_t near_intersection(vec3_t inside, vec3_t outside) {
  const float t = (1.0f - inside.z) / (outside.z - inside.z);
  return (vec3_t){
      .x = inside.x + (outside.x - inside.x) * t,
      .y = inside.y + (outside.y - inside.y) * t,
      .z = 1.0f,
  };
}

static void add_cap_vertex(vec3_t *vertices, int *count, vec3_t point) {
  for (int i = 0; i < *count; i++) {
    if (fabsf(vertices[i].x - point.x) < 0.001f &&
        fabsf(vertices[i].y - point.y) < 0.001f) {
      return;
    }
  }
  if (*count < MAX_CAP_VERTICES) {
    vertices[(*count)++] = point;
  }
}

static int clip_triangle_to_near_plane(const triangle_t *input,
                                       triangle_t *output,
                                       vec3_t *cap_vertices,
                                       int *cap_vertex_count) {
  vec3_t polygon[4];
  int polygon_count = 0;

  for (int i = 0; i < 3; i++) {
    const vec3_t previous = input->v[(i + 2) % 3];
    const vec3_t current = input->v[i];
    const int previous_inside = previous.z <= 1.0f;
    const int current_inside = current.z <= 1.0f;

    if (previous_inside != current_inside) {
      const vec3_t intersection = previous_inside
                                      ? near_intersection(previous, current)
                                      : near_intersection(current, previous);
      polygon[polygon_count++] = intersection;
      add_cap_vertex(cap_vertices, cap_vertex_count, intersection);
    }
    if (current_inside) {
      polygon[polygon_count++] = current;
    }
  }

  if (polygon_count < 3) {
    return 0;
  }
  int triangle_count = 0;
  for (int i = 1; i + 1 < polygon_count; i++) {
    output[triangle_count++] = (triangle_t){
        .v = {polygon[0], polygon[i], polygon[i + 1]},
    };
  }
  return triangle_count;
}

static int append_near_cap(triangle_t *triangles, int triangle_count,
                           vec3_t *vertices, int vertex_count) {
  if (vertex_count < 3) {
    return triangle_count;
  }

  float center_x = 0.0f;
  float center_y = 0.0f;
  for (int i = 0; i < vertex_count; i++) {
    center_x += vertices[i].x;
    center_y += vertices[i].y;
  }
  center_x /= (float)vertex_count;
  center_y /= (float)vertex_count;

  cap_vertex_t sorted[MAX_CAP_VERTICES];
  for (int i = 0; i < vertex_count; i++) {
    sorted[i].p = vertices[i];
    sorted[i].angle = atan2f(vertices[i].y - center_y,
                             vertices[i].x - center_x);
  }
  for (int i = 1; i < vertex_count; i++) {
    const cap_vertex_t value = sorted[i];
    int j = i;
    while (j > 0 && sorted[j - 1].angle > value.angle) {
      sorted[j] = sorted[j - 1];
      j--;
    }
    sorted[j] = value;
  }

  for (int i = 1; i + 1 < vertex_count &&
                  triangle_count < MAX_CLIPPED_TRIANGLES;
       i++) {
    triangles[triangle_count++] = (triangle_t){
        .v = {sorted[0].p, sorted[i + 1].p, sorted[i].p},
    };
  }
  return triangle_count;
}

static int build_clipped_cube(triangle_t *triangles) {
  static const vec3_t cube[8] = {
      {-1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f},
      {1.0f, -1.0f, 1.0f},  {1.0f, -1.0f, -1.0f},
      {-1.0f, 1.0f, 1.0f},  {-1.0f, 1.0f, -1.0f},
      {1.0f, 1.0f, 1.0f},   {1.0f, 1.0f, -1.0f},
  };
  /* Exact 12-triangle topology used by KOS modifier_volume_zclip. */
  static const uint8_t indexes[12][3] = {
      {1, 0, 3}, {0, 3, 2}, {4, 5, 6}, {5, 6, 7},
      {0, 4, 2}, {4, 2, 6}, {2, 6, 3}, {6, 3, 7},
      {3, 7, 1}, {7, 1, 5}, {1, 5, 0}, {5, 0, 4},
  };
  vec3_t transformed[8];
  const float cx = cosf(cube_angle.x);
  const float sx = sinf(cube_angle.x);
  const float cy = cosf(cube_angle.y);
  const float sy = sinf(cube_angle.y);
  const float cz = cosf(cube_angle.z);
  const float sz = sinf(cube_angle.z);

  for (int i = 0; i < 8; i++) {
    /* Scale, rotate around X/Y/Z, translate(-1,.25,1), then apply the
     * perspective camera transform. Each axis advances independently. */
    const float x = cube[i].x * 2.0f;
    const float y = cube[i].y * 2.0f;
    const float z = cube[i].z * 2.0f;

    const float x1 = x;
    const float y1 = y * cx - z * sx;
    const float z1 = y * sx + z * cx;

    const float x2 = x1 * cy + z1 * sy;
    const float y2 = y1;
    const float z2 = -x1 * sy + z1 * cy;

    const float x3 = x2 * cz - y2 * sz;
    const float y3 = x2 * sz + y2 * cz;
    const float z3 = z2;

    const vec3_t world = {
        .x = -1.0f + x3,
        .y = 0.25f + y3,
        .z = 1.0f + z3,
    };
    transformed[i] = project_world(world);
  }

  vec3_t cap_vertices[MAX_CAP_VERTICES];
  int cap_vertex_count = 0;
  int triangle_count = 0;
  for (int i = 0; i < 12; i++) {
    const triangle_t source = {
        .v = {transformed[indexes[i][0]], transformed[indexes[i][1]],
              transformed[indexes[i][2]]},
    };
    triangle_t clipped[2];
    const int clipped_count = clip_triangle_to_near_plane(
        &source, clipped, cap_vertices, &cap_vertex_count);
    for (int j = 0; j < clipped_count &&
                    triangle_count < MAX_CLIPPED_TRIANGLES;
         j++) {
      triangles[triangle_count++] = clipped[j];
    }
  }
  return append_near_cap(triangles, triangle_count, cap_vertices,
                         cap_vertex_count);
}

static void submit_modifier_triangle(const triangle_t *triangle,
                                     uint32_t final_mode) {
  pvr_mod_hdr_t *header = (pvr_mod_hdr_t *)pvr_dr_target();
  pvr_mod_compile(header, receiver_list + 1, final_mode, PVR_CULLING_SMALL);
  pvr_dr_commit(header);

  pvr_modifier_vol_t *first;
  pvr_modifier_vol_t *second;
  enj_draw_pvr_dr64_init((void **)&first, (void **)&second);
  first->flags = PVR_CMD_VERTEX_EOL;
  first->ax = triangle->v[0].x;
  first->ay = triangle->v[0].y;
  first->az = triangle->v[0].z;
  first->bx = triangle->v[1].x;
  first->by = triangle->v[1].y;
  first->bz = triangle->v[1].z;
  first->cx = triangle->v[2].x;
  enj_draw_pvr_dr64_commit_1st();
  second->cy = triangle->v[2].y;
  second->cz = triangle->v[2].z;
  enj_draw_pvr_dr64_commit_2nd();
}

static void submit_receiver_triangle(const triangle_t *triangle,
                                     uint32_t area0, uint32_t area1) {
  for (int i = 0; i < 3; i++) {
    pvr_vertex_pcm_t *vertex = (pvr_vertex_pcm_t *)pvr_dr_target();
    vertex->flags = i == 2 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
    vertex->x = triangle->v[i].x;
    vertex->y = triangle->v[i].y;
    vertex->z = triangle->v[i].z;
    vertex->argb0 = area0;
    vertex->argb1 = area1;
    pvr_dr_commit(vertex);
  }
}

static void render_receiver(void *__unused data) {
  pvr_poly_cxt_t context;
  pvr_poly_cxt_col_mod(&context, receiver_list);
  context.gen.culling = PVR_CULLING_NONE;
  pvr_poly_mod_hdr_t *header = (pvr_poly_mod_hdr_t *)pvr_dr_target();
  pvr_poly_mod_compile(header, &context);
  pvr_dr_commit(header);

  /* A tessellated perspective ground plane stands in for the KOS texture.
   * It supplies many receiver depths under one projected modifier silhouette. */
  for (int z = 0; z < 8; z++) {
    for (int x = 0; x < 10; x++) {
      const float x0 = -5.0f + (float)x;
      const float x1 = x0 + 1.0f;
      const float z0 = -5.0f + (float)z;
      const float z1 = z0 + 1.0f;
      const vec3_t corners[4] = {
          project_world((vec3_t){x0, 0.0f, z0}),
          project_world((vec3_t){x0, 0.0f, z1}),
          project_world((vec3_t){x1, 0.0f, z0}),
          project_world((vec3_t){x1, 0.0f, z1}),
      };
      const uint32_t checker_color =
          ((x + z) & 1) ? 0x0077a9bdu : 0x00b7d4dcu;
      const uint32_t modifier_color =
          receiver_mode == RECEIVER_MODIFIER_TRANSLUCENT
              ? checker_color
              : (((x + z) & 1) ? 0x0024343au : 0x0035454au);
      const uint32_t area0 = receiver_alpha(0) | checker_color;
      /* MIX demonstrates object transparency, not a projected floor shadow. */
      const uint32_t area1_alpha =
          receiver_mode == RECEIVER_MODIFIER_TRANSLUCENT
              ? 0xff000000u
              : receiver_alpha(1);
      const uint32_t area1 = area1_alpha | modifier_color;
      const triangle_t first = {.v = {corners[0], corners[1], corners[2]}};
      const triangle_t second = {.v = {corners[2], corners[1], corners[3]}};
      submit_receiver_triangle(&first, area0, area1);
      submit_receiver_triangle(&second, area0, area1);
    }
  }

  /* Full solid box from the KOS scene: eight vertices and twelve triangles. */
  static const vec3_t cube[8] = {
      {-1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f},
      {1.0f, -1.0f, 1.0f},  {1.0f, -1.0f, -1.0f},
      {-1.0f, 1.0f, 1.0f},  {-1.0f, 1.0f, -1.0f},
      {1.0f, 1.0f, 1.0f},   {1.0f, 1.0f, -1.0f},
  };
  /* Standalone display triangles need consistent winding. The modifier
   * volume's parity-only triangle order is intentionally kept separate. */
  static const uint8_t indexes[12][3] = {
      {1, 0, 3}, {0, 2, 3}, {4, 5, 6}, {5, 7, 6},
      {0, 4, 2}, {4, 6, 2}, {2, 6, 3}, {6, 7, 3},
      {3, 7, 1}, {7, 5, 1}, {1, 5, 0}, {5, 4, 0},
  };
  static const uint32_t face_colors[6] = {
      0xffd77d4du, 0xffe9ad63u, 0xffd89051u,
      0xffbf6848u, 0xffa95442u, 0xffcc7650u,
  };
  const float box_cos = cosf(box_angle);
  const float box_sin = sinf(box_angle);
  vec3_t box[8];
  for (int i = 0; i < 8; i++) {
    const float x = cube[i].x * 2.0f;
    const float z = cube[i].z * 2.0f;
    box[i] = project_world((vec3_t){
        .x = 1.0f + x * box_cos + z * box_sin,
        .y = 2.0f + cube[i].y * 2.0f + BOX_FLOOR_EPSILON,
        .z = -1.0f - x * box_sin + z * box_cos,
    });
  }

  /* Keep the opaque box backface-culled, but expose both sides in translucent
   * mode so its interior remains visible through the shell. */
  pvr_poly_cxt_col_mod(&context, receiver_list);
  context.gen.culling = receiver_list == PVR_LIST_TR_POLY
                            ? PVR_CULLING_NONE
                            : PVR_CULLING_CCW;
  header = (pvr_poly_mod_hdr_t *)pvr_dr_target();
  pvr_poly_mod_compile(header, &context);
  pvr_dr_commit(header);

  for (int i = 0; i < 12; i++) {
    const triangle_t face = {
        .v = {box[indexes[i][0]], box[indexes[i][1]], box[indexes[i][2]]},
    };
    const uint32_t area0 = receiver_alpha(0) |
                           (face_colors[i >> 1] & 0x00ffffffu);
    const uint32_t area1 = receiver_alpha(1) | 0x00402b25u;
    submit_receiver_triangle(&face, area0, area1);
  }
}

static void render_modifier(void *__unused data) {
  triangle_t triangles[MAX_CLIPPED_TRIANGLES];
  const int count = build_clipped_cube(triangles);
  for (int i = 0; i < count; i++) {
    submit_modifier_triangle(
        &triangles[i], i == count - 1 ? PVR_MODIFIER_INCLUDE_LAST_POLY
                                     : PVR_MODIFIER_OTHER_POLY);
  }
}

static vec3_t unproject_camera(vec3_t projected) {
  const float depth = CAMERA_NEAR / projected.z;
  return (vec3_t){
      .x = (projected.x - screen_x_offset - 160.0f) * depth / CAMERA_FOCAL,
      .y = -(projected.y - screen_y_offset - 112.0f) * depth / CAMERA_FOCAL,
      .z = depth,
  };
}

static uint32_t modifier_cube_shade(const triangle_t *triangle) {
  const vec3_t a = unproject_camera(triangle->v[0]);
  const vec3_t b = unproject_camera(triangle->v[1]);
  const vec3_t c = unproject_camera(triangle->v[2]);
  const vec3_t ab = {b.x - a.x, b.y - a.y, b.z - a.z};
  const vec3_t ac = {c.x - a.x, c.y - a.y, c.z - a.z};
  const vec3_t normal = {
      ab.y * ac.z - ab.z * ac.y,
      ab.z * ac.x - ab.x * ac.z,
      ab.x * ac.y - ab.y * ac.x,
  };
  const float length = sqrtf(normal.x * normal.x + normal.y * normal.y +
                             normal.z * normal.z);
  if (length <= 0.000001f) {
    return 0x2400434du;
  }

  /* Upper-left camera-space light. Absolute incidence keeps both sides of
   * this translucent, unculled diagnostic volume legible. */
  const float incidence = fabsf((normal.x * -0.365f + normal.y * 0.548f +
                                  normal.z * -0.753f) /
                                 length);
  const float brightness = fminf(1.0f, 0.28f + incidence * 0.72f);
  const uint32_t red = (uint32_t)(32.0f * brightness);
  const uint32_t green = (uint32_t)(223.0f * brightness);
  const uint32_t blue = (uint32_t)(255.0f * brightness);
  return 0x24000000u | (red << 16) | (green << 8) | blue;
}

static void render_modifier_cube(void *__unused data) {
  pvr_poly_cxt_t context;
  pvr_poly_cxt_col(&context, PVR_LIST_TR_POLY);
  context.gen.culling = PVR_CULLING_NONE;
  context.depth.write = 0;
  pvr_poly_hdr_t *header = (pvr_poly_hdr_t *)pvr_dr_target();
  pvr_poly_compile(header, &context);
  pvr_dr_commit(header);

  triangle_t triangles[MAX_CLIPPED_TRIANGLES];
  const int count = build_clipped_cube(triangles);
  for (int i = 0; i < count; i++) {
    const uint32_t color = modifier_cube_shade(&triangles[i]);
    for (int j = 0; j < 3; j++) {
      pvr_vertex_t *vertex = (pvr_vertex_t *)pvr_dr_target();
      vertex->flags = j == 2 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
      vertex->x = triangles[i].v[j].x;
      vertex->y = triangles[i].v[j].y;
      vertex->z = triangles[i].v[j].z;
      vertex->u = 0.0f;
      vertex->v = 0.0f;
      vertex->argb = color;
      vertex->oargb = 0;
      pvr_dr_commit(vertex);
    }
  }
}

static void render_labels(void *__unused data) {
  enj_font_scale_set(1);
  const char *mode_label = "KOS zclip: opaque depth/stencil";
  if (receiver_mode == RECEIVER_TRANSLUCENT) {
    mode_label = "KOS zclip: transparent per-fragment depth";
  } else if (receiver_mode == RECEIVER_MODIFIER_TRANSLUCENT) {
    mode_label = "KOS zclip: inside alpha, outside opaque";
  }
  enj_qfont_write(mode_label,
                  screen_x_offset + 12,
                  screen_y_offset + 198, PVR_LIST_PT_POLY);
  enj_qfont_write(show_modifier_cube ? "B: modifier cube ON"
                                     : "B: modifier cube OFF",
                  screen_x_offset + 12, screen_y_offset + 210,
                  PVR_LIST_PT_POLY);
  enj_qfont_write("A: OP/TR/MIX; closed XOR cap at z=1",
                  screen_x_offset + 12, screen_y_offset + 222,
                  PVR_LIST_PT_POLY);
}

static void main_mode_updater(void *__unused data) {
  const float two_pi = 6.28318530718f;
  cube_angle.x += 0.007f;
  cube_angle.y += 0.011f;
  cube_angle.z += 0.004f;
  box_angle += 0.0035f;
  if (cube_angle.x >= two_pi) cube_angle.x -= two_pi;
  if (cube_angle.y >= two_pi) cube_angle.y -= two_pi;
  if (cube_angle.z >= two_pi) cube_angle.z -= two_pi;
  if (box_angle >= two_pi) box_angle -= two_pi;
  enj_ctrlr_state_t **controllers = enj_ctrl_get_states();
  for (int i = 0; i < 4; i++) {
    if (controllers[i] != NULL &&
        controllers[i]->button.A == ENJ_BUTTON_DOWN_THIS_FRAME) {
      receiver_mode = (receiver_mode + 1) % RECEIVER_MODE_COUNT;
      receiver_list = receiver_mode == RECEIVER_OPAQUE
                          ? PVR_LIST_OP_POLY
                          : PVR_LIST_TR_POLY;
    }
    if (controllers[i] != NULL &&
        controllers[i]->button.B == ENJ_BUTTON_DOWN_THIS_FRAME) {
      show_modifier_cube = !show_modifier_cube;
    }
  }
  enj_render_list_add(receiver_list, render_receiver, NULL);
  if (modifier_effect_enabled) {
    enj_render_list_add(receiver_list + 1, render_modifier, NULL);
  }
  if (show_modifier_cube) {
    enj_render_list_add(PVR_LIST_TR_POLY, render_modifier_cube, NULL);
  }
  enj_render_list_add(PVR_LIST_PT_POLY, render_labels, NULL);
}

int main(__unused int argc, __unused char **argv) {
  enj_state_init_defaults();
  enj_state_soft_reset_set(ENJ_BUTTON_DOWN << (8 << 1));
  if (enj_state_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }

  screen_x_offset = (float)(vid_mode->width >> 1) - LOGICAL_WIDTH * 0.5f;
  screen_y_offset = (float)(vid_mode->height >> 1) - LOGICAL_HEIGHT * 0.5f;
  pvr_set_bg_color(0.025f, 0.04f, 0.065f);
#ifdef ENJ_DEEP_MODIFIERS_TEST_CONTROLS
  if (getenv("ENJ_MODIFIER_TRANSLUCENT") != NULL) {
    receiver_mode = RECEIVER_TRANSLUCENT;
    receiver_list = PVR_LIST_TR_POLY;
  }
  if (getenv("ENJ_MODIFIER_INSIDE_TRANSLUCENT") != NULL) {
    receiver_mode = RECEIVER_MODIFIER_TRANSLUCENT;
    receiver_list = PVR_LIST_TR_POLY;
  }
  if (getenv("ENJ_MODIFIER_CUBE_HIDDEN") != NULL) {
    show_modifier_cube = 0;
  }
  if (getenv("ENJ_MODIFIER_EFFECT_DISABLED") != NULL) {
    modifier_effect_enabled = 0;
  }
#endif

  enj_mode_t mode = {
      .name = "Deep Modifiers",
      .mode_updater = main_mode_updater,
      .data = NULL,
  };
  enj_mode_push(&mode);
  enj_state_run();
  return 0;
}
