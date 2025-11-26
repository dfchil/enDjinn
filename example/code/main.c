#include <dc/fmath.h>
#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>
#include <math.h>

static const alignas(32) uint8_t enDjinn_txt_raw[] = {
#embed "../embeds/example/texture/pal8/enDjinn512.dt"
};
static const alignas(32) uint8_t enDjinn_palette_raw[] = {
#embed "../embeds/example/texture/pal8/enDjinn512.dt.pal"
};
static enj_texture_info_t figure_texture_info;

static const alignas(32) uint8_t help_txt_raw[] = {
#embed "../embeds/example/texture/pal8/info512.dt"
};
static const alignas(32) uint8_t help_palette_raw[] = {
#embed "../embeds/example/texture/pal8/info512.dt.pal"
};
static enj_texture_info_t help_texture_info;

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif

typedef struct {
  uint32_t rotation;
  pvr_sprite_hdr_t hdr;
  union {
    uint32_t raw; // 0xAABBGGRR
    struct {
      /* Intensity color */
      uint8_t b; /**< Intensity color alpha */
      uint8_t g; /**< Intensity color red */
      uint8_t r; /**< Intensity color green */
      uint8_t a; /**< Intensity color blue */
    };
  } spec_color;
  float base_size;
  float center_x;
  float center_y;
  int32_t size_bump;
} mode_data_t;

typedef struct {
  pvr_sprite_hdr_t hdr;
} info_mode_data_t;

typedef struct {
  mode_data_t *mode_data;
  float velocity_x;
  float velocity_y;
} shutdown_mode_data_t;

alignas(32) static enj_mode_t startup_mode = {.name = "Startup Mode"};
alignas(32) static enj_mode_t shutdown_mode = {
    .name = "Shutdown Mode",
};
alignas(32) static info_mode_data_t info_mode_data;
alignas(32) static enj_mode_t info_mode = {.data = &info_mode_data,
                                           .name = "Info Mode"};

static inline void rotate2d(float x, float y, float sin, float cos,
                            float *out_x, float *out_y) {
  *out_x = (x * cos - y * sin) * ENJ_XSCALE;
  *out_y = x * sin + y * cos;
}

void draw_quad(pvr_sprite_hdr_t *hdr, float corners[4][2], float z_value) {
  pvr_dr_state_t dr_state;
  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t *mode_hdr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
  *mode_hdr = *hdr;
  pvr_dr_commit(mode_hdr);
  pvr_sprite_txr_t *quad = (pvr_sprite_txr_t *)pvr_dr_target(dr_state);
  quad->flags = PVR_CMD_VERTEX_EOL;
  quad->ax = corners[0][0];
  quad->ay = corners[0][1];
  quad->az = z_value;
  quad->bx = corners[1][0];
  quad->by = corners[1][1];
  quad->bz = z_value;
  quad->cx = corners[2][0];
  pvr_dr_commit(quad);
  quad = (pvr_sprite_txr_t *)pvr_dr_target(dr_state);
  pvr_sprite_txr_t *quad2ndhalf = (pvr_sprite_txr_t *)((int)quad - 32);
  quad2ndhalf->cy = corners[2][1];
  quad2ndhalf->cz = z_value;
  quad2ndhalf->dx = corners[3][0];
  quad2ndhalf->dy = corners[3][1];
  quad2ndhalf->auv = PVR_PACK_16BIT_UV(0.0f, 1.0f);
  quad2ndhalf->buv = PVR_PACK_16BIT_UV(0.0f, 0.0f);
  quad2ndhalf->cuv = PVR_PACK_16BIT_UV(1.0f, 0.0f);
  pvr_dr_commit(quad);
  pvr_dr_finish();
}

void shared_renderer(void *data) {
  mode_data_t *mdata = (mode_data_t *)data;
  float cur_radius = mdata->base_size - mdata->size_bump;

  float angle = -((mdata->rotation % 360) * (F_PI / 180.0f));
  float cos, sin;
  fsincosr(angle, &sin, &cos);

  float corners[4][2] = {
      {-cur_radius, -cur_radius},
      {-cur_radius, cur_radius},
      {cur_radius, cur_radius},
      {cur_radius, -cur_radius},
  };
  for (int i = 0; i < 4; i++) {
    float x = corners[i][0];
    float y = corners[i][1];
    rotate2d(x, y, sin, cos, &corners[i][0], &corners[i][1]);
    corners[i][0] += mdata->center_x;
    corners[i][1] += mdata->center_y;
  }
  draw_quad(&mdata->hdr, corners, 1.0f);
}

void info_renderer(void *data) {
  info_mode_data_t *mdata = (info_mode_data_t *)data;

  float min_x = (vid_mode->width - help_texture_info.width) * 0.5f * ENJ_XSCALE;
  float max_x = min_x + help_texture_info.width * ENJ_XSCALE;
  float min_y = (vid_mode->height - 440) * 0.5f;
  float max_y = min_y + help_texture_info.height; 

  float corners[4][2] = {
      {min_x, max_y},
      {min_x, min_y},
      {max_x, min_y},
      {max_x, max_y},
  };
  draw_quad(&mdata->hdr, corners, 2.0f);
}
static inline void shared_animation(mode_data_t *mdata) {
  mdata->size_bump = MAX(0, mdata->size_bump - 1);
  mdata->rotation++;
  mdata->spec_color.r = MAX(mdata->spec_color.r - 1, 0);
  mdata->spec_color.g = MAX(mdata->spec_color.g - 1, 0);
  mdata->spec_color.b = MAX(mdata->spec_color.b - 1, 0);
  mdata->hdr.oargb = mdata->spec_color.raw;
}

void info_updater(void *data) {
  enj_ctrlr_state_t **ctrls = enj_ctrl_get_states();
  for (int i = 0; i < 4; i++) {
    if (ctrls[i] != NULL) {
      if (ctrls[i]->buttons.START == BUTTON_DOWN_THIS_FRAME) {
        enj_mode_flag_end_current();
      }
    }
  }
  shared_animation(startup_mode.data);

  enj_renderlist_add(PVR_LIST_TR_POLY, info_renderer, data);
  enj_renderlist_add(PVR_LIST_TR_POLY, shared_renderer, startup_mode.data); 
}


void shutdown_default_direction(enj_mode_t *prev, enj_mode_t *next) {
  shutdown_mode_data_t *smdata = (shutdown_mode_data_t *)next->data;
  smdata->velocity_x = 0.0f;
  smdata->velocity_y = +9.5f;
}

void into_main_indicator(enj_mode_t *prev, enj_mode_t *next) {
  mode_data_t *mdata = (mode_data_t *)next->data;
  mdata->size_bump = 5;
  mdata->spec_color.raw = 0xff2f2f2f;
  mdata->center_x = vid_mode->width * ENJ_XSCALE * 0.5f;
  mdata->center_y = vid_mode->height * 0.5f;
}

void startup_mode_initializer(enj_mode_t *prev, enj_mode_t *next) {
  mode_data_t *mdata = (mode_data_t *)next->data;
  into_main_indicator(prev, next);
  mdata->size_bump = mdata->base_size;
}

void startup_mode_updater(void *data) {
  mode_data_t *mdata = (mode_data_t *)data;
  shared_animation(mdata);
  if (mdata->size_bump <= 0) {
    enj_mode_flag_end_current();
  } else {
    mdata->size_bump--;
  }
  enj_renderlist_add(PVR_LIST_TR_POLY, shared_renderer, data);
}

void shutdown_mode_updater(void *data) {
  shutdown_mode_data_t *smdata = (shutdown_mode_data_t *)data;
  shared_animation(smdata->mode_data);
  smdata->mode_data->center_x += smdata->velocity_x;
  smdata->mode_data->center_y += smdata->velocity_y;
  if (smdata->mode_data->center_x <
          -figure_texture_info.width * ENJ_XSCALE * 0.5f ||
      smdata->mode_data->center_x >
          (vid_mode->width + figure_texture_info.width) * ENJ_XSCALE ||
      smdata->mode_data->center_y < -figure_texture_info.height * 0.5f ||
      smdata->mode_data->center_y >
          vid_mode->height + figure_texture_info.height * 0.5f) {
    enj_mode_flag_end_current();
  }
  enj_renderlist_add(PVR_LIST_TR_POLY, shared_renderer, smdata->mode_data);
}

void main_mode_updater(void *data) {
  mode_data_t *mdata = (mode_data_t *)data;
  shared_animation(mdata);

  enj_ctrlr_state_t **ctrls = enj_ctrl_get_states();
  for (int i = 0; i < 4; i++) {
    if (ctrls[i] != NULL) {
      if (ctrls[i]->buttons.A == BUTTON_DOWN_THIS_FRAME) {
        mdata->spec_color.r = MIN(mdata->spec_color.r + 0x3f, 0xff);
        mdata->size_bump += 15;
      }
      if (ctrls[i]->buttons.X == BUTTON_DOWN_THIS_FRAME) {
        mdata->spec_color.r = MIN(mdata->spec_color.r + 0x1f, 0xff);
        mdata->spec_color.g = MIN(mdata->spec_color.g + 0x1f, 0xff);
        mdata->size_bump += 15;
      }
      if (ctrls[i]->buttons.Y == BUTTON_DOWN_THIS_FRAME) {
        mdata->spec_color.g = MIN(mdata->spec_color.g + 0x3f, 0xff);
        mdata->size_bump += 15;
      }
      if (ctrls[i]->buttons.B == BUTTON_DOWN_THIS_FRAME) {
        mdata->spec_color.raw = 0xff000000;
        mdata->size_bump += 15;
      }
      if (ctrls[i]->buttons.START == BUTTON_DOWN_THIS_FRAME) {
        enj_mode_push(&info_mode);
      }

      if (ABS(ctrls[i]->joyx) + ABS(ctrls[i]->joyy) > 30) {
        mdata->rotation =
            (2.0f * F_PI + atan2f(ctrls[i]->joyy, -ctrls[i]->joyx + 0.0001f)) *
            (180.0f / F_PI);
      }
      uint32_t dpad = BUTTON_DOWN_THIS_FRAME
                      << 8; // ie dpad DOWN pressed this frame
      for (int d = 0; d < 4; d++) {
        if ((ctrls[i]->buttons.raw & dpad) == dpad) {
          enj_mode_push(&startup_mode); // this will run after shutdown mode
          enj_mode_push(&shutdown_mode);
          shutdown_mode_data_t *sd = (shutdown_mode_data_t *)shutdown_mode.data;
          sd->velocity_x = sd->velocity_y = 0.0f;
          switch (d) {
          case 0: // UP
            sd->velocity_y = -7.5f;
            break;
          case 1: // DOWN
            sd->velocity_y = +7.5f;
            break;
          case 2: // LEFT
            sd->velocity_x = -7.5f * ENJ_XSCALE;
            break;
          case 3: // RIGHT
            sd->velocity_x = +7.5f * ENJ_XSCALE;
            break;
          default:
            break;
          };
          break; // only handle one dpad press per frame
        }
        dpad <<= 2;
      }
    }
  }
  enj_renderlist_add(PVR_LIST_TR_POLY, shared_renderer, data);
}

void setup_textures() {
  // load textures from memory blobs
  enj_texture_load_blob(enDjinn_txt_raw, &figure_texture_info);
  enj_texture_bind_palette(&figure_texture_info, 0);
  enj_texture_load_palette_blob(enDjinn_palette_raw,
                                figure_texture_info.flags.palette_format,
                                figure_texture_info.flags.palette_position);

  enj_texture_load_blob(help_txt_raw, &help_texture_info);
  enj_texture_bind_palette(&help_texture_info, 256);
  enj_texture_load_palette_blob(help_palette_raw,
                                help_texture_info.flags.palette_format,
                                help_texture_info.flags.palette_position);
}

void setup_modes(enj_mode_t *main_mode,
                 shutdown_mode_data_t *shutdown_mode_data) {
  // setup info mode
  pvr_sprite_cxt_t i_cxt;
  pvr_sprite_cxt_txr(&i_cxt, PVR_LIST_TR_POLY, help_texture_info.pvrformat,
                     help_texture_info.width, help_texture_info.height,
                     help_texture_info.ptr, PVR_FILTER_BILINEAR);
  i_cxt.gen.culling = PVR_CULLING_NONE;
  pvr_sprite_compile(&info_mode_data.hdr, &i_cxt);
  info_mode.data = &info_mode_data;
  info_mode.mode_updater = info_updater;
  info_mode.pop_fun = NULL;


  // setup the other modes
  mode_data_t *main_mode_data = (mode_data_t *)main_mode->data;
  pvr_sprite_cxt_t f_cxt;
  pvr_sprite_cxt_txr(&f_cxt, PVR_LIST_TR_POLY, figure_texture_info.pvrformat,
                     figure_texture_info.width, figure_texture_info.height,
                     figure_texture_info.ptr, PVR_FILTER_BILINEAR);
  f_cxt.gen.culling = PVR_CULLING_NONE;
  f_cxt.gen.specular = PVR_SPECULAR_ENABLE;
  pvr_sprite_compile(&main_mode_data->hdr, &f_cxt);
  main_mode_data->hdr.argb = 0xffffffff;

  startup_mode.data = main_mode->data;
  startup_mode.mode_updater = startup_mode_updater;
  startup_mode.pop_fun = startup_mode_initializer;

  shutdown_mode_data->mode_data = main_mode_data;
  shutdown_mode.data = shutdown_mode_data;
  shutdown_mode.mode_updater = shutdown_mode_updater;
  shutdown_mode.pop_fun = shutdown_default_direction;

  enj_mode_push(&shutdown_mode);
  enj_mode_push(main_mode);
  enj_mode_set_soft_reset_target(enj_mode_get_current_index());
  enj_mode_push(&startup_mode);
  // pushing doesnt trigger pop function, so call it manually
  startup_mode.pop_fun(NULL, &startup_mode);
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
  enj_state_set_soft_reset((enj_ctrlr_state_t){
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

  setup_textures();
  /* setup at enDjinn modes */
  mode_data_t main_mode_data = {
      .rotation = 0,
      .spec_color = 0xff000000,
      .size_bump = 0,
      .base_size = (figure_texture_info.width) * 0.42f,
  };
  enj_mode_t main_mode = {
      .name = "Main Mode",
      .mode_updater = main_mode_updater,
      .data = &main_mode_data,
      .pop_fun = into_main_indicator,
  };
  shutdown_mode_data_t shutdown_mode_data;

  setup_modes(&main_mode, &shutdown_mode_data);

  enj_run();

  enj_texture_unload(&figure_texture_info);
  enj_texture_unload(&help_texture_info);
  return 0;
}
