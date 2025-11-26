#include <dc/fmath.h>
#include <math.h>
#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>

static const alignas(32) uint8_t texture32_raw[] = {
#embed "../embeds/example/texture/pal8/enDjinn512.dt"
};
static const alignas(32) uint8_t palette32_raw[] = {
#embed "../embeds/example/texture/pal8/enDjinn512.dt.pal"
};
static enj_texture_info_t texture_info;

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
      uint32_t raw;
      struct {
          /* Intensity color */
          uint8_t a;                               /**< Intensity color alpha */
          uint8_t r;                               /**< Intensity color red */
          uint8_t g;                               /**< Intensity color green */
          uint8_t b;                               /**< Intensity color blue */
      };
    } spec_color;
    float base_size;
    float center_x;
    float center_y;
    int32_t size_bump;
} mode_data_t;

typedef struct {
  mode_data_t* mode_data;
  float velocity_x;
  float velocity_y;
} shutown_mode_data_t;

static inline void rotate2d(float x, float y, float sin, float cos,
                            float* out_x, float* out_y) {
    *out_x = (x * cos - y * sin) * ENJ_XSCALE;
    *out_y = x * sin + y * cos;
}
void shared_renderer(void* data) {
    mode_data_t* mdata = (mode_data_t*)data;
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
    }

    pvr_dr_state_t dr_state;
    pvr_dr_init(&dr_state);
    pvr_sprite_hdr_t* mode_data_hdr =
        (pvr_sprite_hdr_t*)pvr_dr_target(dr_state);
    *mode_data_hdr = mdata->hdr;
    pvr_dr_commit(mode_data_hdr);
    pvr_sprite_txr_t* quad = (pvr_sprite_txr_t*)pvr_dr_target(dr_state);
    quad->flags = PVR_CMD_VERTEX_EOL;
    quad->ax = mdata->center_x + corners[0][0];
    quad->ay = mdata->center_y + corners[0][1];
    quad->az = 1.0f;
    quad->bx = mdata->center_x + corners[1][0];
    quad->by = mdata->center_y + corners[1][1];
    quad->bz = 1.0f;
    quad->cx = mdata->center_x + corners[2][0];
    pvr_dr_commit(quad);
    quad = (pvr_sprite_txr_t*)pvr_dr_target(dr_state);
    pvr_sprite_txr_t* quad2ndhalf = (pvr_sprite_txr_t*)((int)quad - 32);
    quad2ndhalf->cy = mdata->center_y + corners[2][1];
    quad2ndhalf->cz = 1.0f;
    quad2ndhalf->dx = mdata->center_x + corners[3][0];
    quad2ndhalf->dy = mdata->center_y + corners[3][1];
    quad2ndhalf->auv = PVR_PACK_16BIT_UV(1.0f, 0.0f);
    quad2ndhalf->buv = PVR_PACK_16BIT_UV(0.0f, 0.0f);
    quad2ndhalf->cuv = PVR_PACK_16BIT_UV(0.0f, 1.0f);
    pvr_dr_commit(quad);
    pvr_dr_finish();
}

static inline void shared_animation(mode_data_t* mdata) {
    mdata->size_bump = MAX(0, mdata->size_bump - 1);
    mdata->rotation++;
    mdata->spec_color.r = MAX(mdata->spec_color.r - 1, 0);
    mdata->spec_color.g = MAX(mdata->spec_color.g - 1, 0);
    mdata->spec_color.b = MAX(mdata->spec_color.b - 1, 0);
    mdata->hdr.oargb = mdata->spec_color.raw;
}

void startup_mode_initializer(enj_mode_t* prev, enj_mode_t* next) {
  mode_data_t* mdata = (mode_data_t*)prev->data;
  mdata->size_bump = mdata->base_size;
}

void shutdown_default_direction(enj_mode_t* prev, enj_mode_t* next) {
  shutown_mode_data_t* smdata = (shutown_mode_data_t*)next->data;
  smdata->velocity_x = 0.0f;
  smdata->velocity_y = +4.5f;
}

void startup_mode_updater(void* data) {
  mode_data_t* mdata = (mode_data_t*)data;
  shared_animation(mdata);
  if (mdata->size_bump <= 0) {
    enj_mode_flag_end_current();
  }
  else {
    mdata->size_bump--;
  }
  printf("Size bump: %d\n", mdata->size_bump);
  enj_renderlist_add(PVR_LIST_TR_POLY, shared_renderer, data);
}

void shutdown_mode_updater(void* data) {
  shutown_mode_data_t* smdata = (shutown_mode_data_t*)data;
  shared_animation(smdata->mode_data);
  smdata->mode_data->center_x += smdata->velocity_x;
  smdata->mode_data->center_y += smdata->velocity_y;
  if (smdata->mode_data->center_x < -texture_info.width * ENJ_XSCALE ||
      smdata->mode_data->center_x > (vid_mode->width + texture_info.width) * ENJ_XSCALE ||
      smdata->mode_data->center_y < - texture_info.height ||
      smdata->mode_data->center_y > vid_mode->height + texture_info.height) {
    enj_mode_flag_end_current();
  }
  enj_renderlist_add(PVR_LIST_TR_POLY, shared_renderer, smdata->mode_data);
}

void main_mode_updater(void* data) {
    mode_data_t* mdata = (mode_data_t*)data;

    shared_animation(mdata);
    enj_ctrlr_state_t** ctrls = enj_ctrl_get_states();
    for (int i = 0; i < 4; i++) {
        if (ctrls[i] != NULL) {
            if (ctrls[i]->buttons.A == BUTTON_DOWN_THIS_FRAME) {
                mdata->spec_color.r = MIN(mdata->spec_color.r + 0x3f, 0xff);
                mdata->size_bump+=15;
              }
            if (ctrls[i]->buttons.X == BUTTON_DOWN_THIS_FRAME) {
                mdata->spec_color.r = MIN(mdata->spec_color.r + 0x1f, 0xff);
                mdata->spec_color.g = MIN(mdata->spec_color.g + 0x1f, 0xff);
                mdata->size_bump+=15;
            }
            if (ctrls[i]->buttons.Y == BUTTON_DOWN_THIS_FRAME) {
                mdata->spec_color.g = MIN(mdata->spec_color.g + 0x3f, 0xff);
                mdata->size_bump+=15;

            }
            if (ctrls[i]->buttons.B == BUTTON_DOWN_THIS_FRAME) {
                mdata->spec_color.raw = 0xff000000;
                mdata->size_bump+=15;
            }
            if (ABS(ctrls[i]->joyx) + ABS(ctrls[i]->joyy) > 30) {
              mdata->rotation = atan2f(-ctrls[i]->joyy, ctrls[i]->joyx + 0.0001f) * (180.0f / F_PI);
            }
        }
    }
    enj_renderlist_add(PVR_LIST_TR_POLY, shared_renderer, data);
}

int main(__unused int argc, __unused char** argv) {
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
    }.buttons.raw);

    if (enj_startup() != 0) {
        ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
        return -1;
    }

    // load a single paletised texture from memory blobs
    enj_texture_load_blob(texture32_raw, &texture_info);
    enj_texture_load_palette_blob(palette32_raw, PVR_PAL_ARGB8888, 0);

    /* setup at a single enDjinn mode */
    mode_data_t main_mode_data = {
        .rotation = 0,
        .spec_color = 0xff000000,
        .size_bump = 0,
        .center_x = vid_mode->width * ENJ_XSCALE * 0.5f,
        .center_y = vid_mode->height * 0.5f
    };
    
    pvr_sprite_cxt_t cxt;
    pvr_sprite_cxt_txr(&cxt, PVR_LIST_TR_POLY, texture_info.pvrformat,
                       texture_info.width, texture_info.height,
                       texture_info.ptr, PVR_FILTER_BILINEAR);
    cxt.gen.culling = PVR_CULLING_NONE;
    cxt.gen.specular = PVR_SPECULAR_ENABLE;
    pvr_sprite_compile(&main_mode_data.hdr, &cxt);
    main_mode_data.hdr.argb = 0xffffffff;

    enj_mode_t main_mode = {
        .name = "Main Mode",
        .mode_updater = main_mode_updater,
        .data = &main_mode_data,
    };
    enj_mode_t startup_mode = {
        .name = "Startup Mode",
        .mode_updater = startup_mode_updater,
        .data = &main_mode_data,
        .trans_fun = startup_mode_initializer,
    };
    enj_mode_t shutdown_mode = {
        .name = "Shutdown Mode",
        .mode_updater = shutdown_mode_updater,
        .data = &(shutown_mode_data_t){
            .mode_data = &main_mode_data,
        },
        .trans_fun = shutdown_default_direction
    };
    enj_mode_push(&shutdown_mode);
    enj_mode_push(&main_mode);
    enj_mode_push(&startup_mode);
    enj_run();

    enj_texture_unload(&texture_info);
    return 0;
}