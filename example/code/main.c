#include <dc/fmath.h>
#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>

static const alignas(32) uint8_t texture32_raw[] = {
#embed "../embeds/example/texture/pal8/enDjinn512.dt"
};
static const alignas(32) uint8_t palette32_raw[] = {
#embed "../embeds/example/texture/pal8/enDjinn512.dt.pal"
};
static enj_texture_info_t texture_info;

typedef struct {
  uint32_t counter;
  pvr_sprite_hdr_t hdr;
} main_mode_data_t;

static inline void rotate2d(float x, float y, float sin, float cos,
                            float *out_x, float *out_y) {
  *out_x = (x * cos - y * sin) * ENJ_XSCALE;
  *out_y = x * sin + y * cos;
}

void render_main_mode(void *data) {
  main_mode_data_t *mode_data = (main_mode_data_t *)data;
  float center_x = vid_mode->width * ENJ_XSCALE * 0.5f;
  float center_y = vid_mode->height * 0.5f;
  float half_w = (texture_info.width) * 0.42f;
  float half_h = (texture_info.height) * 0.42;

  // float corner_dist = fsqrt(half_w * half_w + half_h * half_h);
  float angle = -((mode_data->counter % 360) * (F_PI / 180.0f));
  float cos = fcos(angle);
  float sin = fsin(angle);

  float corners[4][2] = {
      {-half_w, -half_h},
      {-half_w, half_h},
      {half_w, half_h},
      {half_w, -half_h},
  };
  for (int i = 0; i < 4; i++) {
    float x = corners[i][0];
    float y = corners[i][1];
    rotate2d(x, y, sin, cos, &corners[i][0], &corners[i][1]);
  }

  pvr_dr_state_t dr_state;
  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t *mode_data_hdr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
  *mode_data_hdr = mode_data->hdr;
  pvr_dr_commit(mode_data_hdr);
  pvr_sprite_txr_t *quad = (pvr_sprite_txr_t *)pvr_dr_target(dr_state);
  quad->flags = PVR_CMD_VERTEX_EOL;
  quad->ax = center_x + corners[0][0];
  quad->ay = center_y + corners[0][1];
  quad->az = 1.0f;
  quad->bx = center_x + corners[1][0];
  quad->by = center_y + corners[1][1];
  quad->bz = 1.0f;
  quad->cx = center_x + corners[2][0];
  pvr_dr_commit(quad);
  quad = (pvr_sprite_txr_t *)pvr_dr_target(dr_state);
  pvr_sprite_txr_t *quad2ndhalf = (pvr_sprite_txr_t *)((int)quad - 32);
  quad2ndhalf->cy = center_y + corners[2][1];
  quad2ndhalf->cz = 1.0f;
  quad2ndhalf->dx = center_x + corners[3][0];
  quad2ndhalf->dy = center_y + corners[3][1];
  quad2ndhalf->auv = PVR_PACK_16BIT_UV(1.0f, 0.0f);
  quad2ndhalf->buv = PVR_PACK_16BIT_UV(0.0f, 0.0f);
  quad2ndhalf->cuv = PVR_PACK_16BIT_UV(0.0f, 1.0f);
  pvr_dr_commit(quad);
  pvr_dr_finish();
}

void main_mode_updater(void *data) {
  main_mode_data_t *mode_data = (main_mode_data_t *)data;
  mode_data->counter++;

  enj_renderlist_add(PVR_LIST_TR_POLY, render_main_mode, data);
}

int main(__unused int argc, __unused char **argv) {

  // initialize enDjinn state with default values
  // you could also work directly on 
  // the enj_state_t* from enj_state_get()
  enj_state_defaults();

  // default pattern is START + A + B + X + Y held down
  // lets make it easier
  // the reason for using the enj_ctrlr_state_t interface
  // for making the bit-pattern is that each button is
  // mapped to 2 bits, one for tracking position and one 
  // tracking if there was a change between readings
  enj_state_set_exit_pattern((enj_ctrlr_state_t){
      .buttons =
          {
              .START = BUTTON_DOWN,
              .A = BUTTON_DOWN,
          },
  }
  .buttons.raw);

  if (enj_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }

  // load a single paletised texture from memory blobs
  enj_texture_load_blob(texture32_raw, &texture_info);
  enj_texture_load_palette_blob(palette32_raw, PVR_PAL_ARGB8888, 0);

  /* setup at a single enDjinn mode */
  main_mode_data_t main_mode_data = {
      .counter = 0,
  };
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_txr(&cxt, PVR_LIST_TR_POLY, texture_info.pvrformat,
                     texture_info.width, texture_info.height, texture_info.ptr,
                     PVR_FILTER_BILINEAR);
  cxt.gen.culling = PVR_CULLING_NONE;
  pvr_sprite_compile(&main_mode_data.hdr, &cxt);
  main_mode_data.hdr.argb = 0xffffffff;
  enj_mode_t main_mode = {
      .name = "Main Mode",
      .mode_updater = main_mode_updater,
      .data = &main_mode_data,
  };
  enj_mode_push(&main_mode);
  enj_run();

  enj_texture_unload(&texture_info);
  return 0;
}
