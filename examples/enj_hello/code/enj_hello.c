#include <dc/fmath.h>
#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>
#include <math.h>

static const alignas(32) uint8_t enj_txr_blob[] = {
#embed "../embeds/enj_hello/texture/pal8/enDjinn512.dt"
};
static const alignas(32) uint8_t enj_palette_blob[] = {
#embed "../embeds/enj_hello/texture/pal8/enDjinn512.dt.pal"
};
static enj_texture_info_t figure_texture_info;
typedef struct {
  uint32_t rotation;
  pvr_sprite_hdr_t hdr;
  float base_size;
  float center_x;
  float center_y;
} main_data_t;

static inline void rotate2d(float x, float y, float sin, float cos,
                            float *out_x, float *out_y) {
  *out_x = (x * cos - y * sin) * ENJ_XSCALE;
  *out_y = x * sin + y * cos;
}
void render(void *data) {
  main_data_t *mdata = (main_data_t *)data;
  float cos, sin;
  fsincosr(-((mdata->rotation % 360) * (F_PI / 180.0f)), &sin, &cos);
  alignas(32) float corners[4][3] = {
      {-mdata->base_size, +mdata->base_size, 1.0f},
      {-mdata->base_size, -mdata->base_size, 1.0f},
      {+mdata->base_size, -mdata->base_size, 1.0f},
      {+mdata->base_size, +mdata->base_size, 1.0f},
  };
  for (int i = 0; i < 4; i++) {
    float x = corners[i][0];
    float y = corners[i][1];
    rotate2d(x, y, sin, cos, &corners[i][0], &corners[i][1]);
    corners[i][0] += mdata->center_x;
    corners[i][1] += mdata->center_y;
  }
  enj_draw_sprite(corners, NULL, &mdata->hdr, NULL);
}
void main_mode_updater(void *data) {
  main_data_t *mdata = (main_data_t *)data;
  mdata->rotation++;
  enj_renderlist_add(PVR_LIST_TR_POLY, render, data);
}
void setup_textures() {
  // load palettised texture from memory blobs
  enj_texture_load_blob(enj_txr_blob, &figure_texture_info);
  enj_texture_bind_palette(&figure_texture_info, 0);
  enj_texture_load_palette_blob(enj_palette_blob,
                                figure_texture_info.flags.palette_format,
                                figure_texture_info.flags.palette_position);
}
void setup_modes(enj_mode_t *main_mode) {
  // setup the other modes
  main_data_t *main_mode_data = (main_data_t *)main_mode->data;
  pvr_sprite_cxt_t f_cxt;
  pvr_sprite_cxt_txr(&f_cxt, PVR_LIST_TR_POLY, figure_texture_info.pvrformat,
                     figure_texture_info.width, figure_texture_info.height,
                     figure_texture_info.ptr, PVR_FILTER_BILINEAR);
  pvr_sprite_compile(&main_mode_data->hdr, &f_cxt);
  main_mode_data->hdr.argb = 0xffffffff;
}

int main(__unused int argc, __unused char **argv) {

  #ifdef ENJ_DIR
  ENJ_DEBUG_PRINT("enDjinn directory: %s\n", ENJ_DIR);
  #endif

  // initialize enDjinn state with default values
  enj_state_defaults();
  // default soft-reset pattern is START + A + B + X + Y.
  // Lets make it easier with just START + A.
  // A is offset 0 in bitfield and START is offset 
  // 8<<1 (two bits per button)
  enj_state_set_soft_reset(BUTTON_DOWN << (8 << 1) | BUTTON_DOWN);
  if (enj_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }
  setup_textures();
  /* setup at enDjinn modes */
  main_data_t main_mode_data = {
      .rotation = 0,
      .base_size = (figure_texture_info.width) * 0.42f,
      .center_x = vid_mode->width * ENJ_XSCALE * 0.5f,
      .center_y = vid_mode->height * 0.5f,
  };
  enj_mode_t main_mode = {
      .name = "Main Mode",
      .mode_updater = main_mode_updater,
      .data = &main_mode_data,
  };
  setup_modes(&main_mode);
  enj_mode_push(&main_mode);
  enj_run();
  enj_texture_unload(&figure_texture_info);
  return 0;
}
