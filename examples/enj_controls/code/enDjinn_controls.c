#include <dc/fmath.h>
#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>
#include <math.h>

static const alignas(32) uint8_t enDjinn_txt_raw[] = {
#embed "../embeds/enDjinn_controls/texture/pal8/enDjinn512.dt"
};
static const alignas(32) uint8_t enDjinn_palette_raw[] = {
#embed "../embeds/enDjinn_controls/texture/pal8/enDjinn512.dt.pal"
};
static enj_texture_info_t figure_texture_info;

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
        uint32_t raw;  // 0xAABBGGRR
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
} main_data_t;

static inline void rotate2d(float x, float y, float sin, float cos,
                            float* out_x, float* out_y) {
    *out_x = (x * cos - y * sin) * ENJ_XSCALE;
    *out_y = x * sin + y * cos;
}

void render(void* data) {
    main_data_t* mdata = (main_data_t*)data;
    float cos, sin;
    fsincosr(-((mdata->rotation % 360) * (F_PI / 180.0f)), &sin, &cos);
    float cur_radius = mdata->base_size - mdata->size_bump;

    float corners[4][3] = {
        {-cur_radius, cur_radius, 1.0f},
        {-cur_radius, -cur_radius, 1.0f},
        {cur_radius, -cur_radius, 1.0f},
        {cur_radius, cur_radius, 1.0f},
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

static inline void animate(main_data_t* mdata) {
    mdata->size_bump = MAX(0, mdata->size_bump - 1);
    mdata->rotation++;
    mdata->spec_color.r = MAX(mdata->spec_color.r - 1, 0);
    mdata->spec_color.g = MAX(mdata->spec_color.g - 1, 0);
    mdata->spec_color.b = MAX(mdata->spec_color.b - 1, 0);
    mdata->hdr.oargb = mdata->spec_color.raw;
}

void main_mode_updater(void* data) {
    main_data_t* mdata = (main_data_t*)data;
    animate(mdata);

    enj_ctrlr_state_t** ctrls = enj_ctrl_get_states();
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
            if (ABS(ctrls[i]->joyx) + ABS(ctrls[i]->joyy) > 30) {
                mdata->rotation =
                    (2.5f * F_PI +
                     atan2f(ctrls[i]->joyy, -ctrls[i]->joyx + 0.0001f)) *
                    (180.0f / F_PI);
            }
        }
    }
    enj_renderlist_add(PVR_LIST_TR_POLY, render, data);
}

void setup_textures() {
    // load textures from memory blobs
    enj_texture_load_blob(enDjinn_txt_raw, &figure_texture_info);
    enj_texture_bind_palette(&figure_texture_info, 0);
    enj_texture_load_palette_blob(enDjinn_palette_raw,
                                  figure_texture_info.flags.palette_format,
                                  figure_texture_info.flags.palette_position);
}

void setup_modes(enj_mode_t* main_mode) {
    main_data_t* main_mode_data = (main_data_t*)main_mode->data;
    pvr_sprite_cxt_t f_cxt;
    pvr_sprite_cxt_txr(&f_cxt, PVR_LIST_TR_POLY, figure_texture_info.pvrformat,
                       figure_texture_info.width, figure_texture_info.height,
                       figure_texture_info.ptr, PVR_FILTER_BILINEAR);
    f_cxt.gen.culling = PVR_CULLING_NONE;
    f_cxt.gen.specular = PVR_SPECULAR_ENABLE;
    pvr_sprite_compile(&main_mode_data->hdr, &f_cxt);
    main_mode_data->hdr.argb = 0xffffffff;    
}

int main(__unused int argc, __unused char** argv) {
    // initialize enDjinn state with default values
    enj_state_defaults();

    // default pattern is START + A + B + X + Y
    // lets make it easier, A is offset 0 in bitfield and START is 
    // offset 8<<1 (two bits per button)
    enj_state_set_soft_reset(BUTTON_DOWN<<(8<<1) | BUTTON_DOWN);

    if (enj_startup() != 0) {
        ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
        return -1;
    }

    setup_textures();
    /* setup at enDjinn modes */
    main_data_t main_mode_data = {
        .rotation = 0,
        .spec_color.raw = 0xff000000,
        .size_bump = 0,
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
