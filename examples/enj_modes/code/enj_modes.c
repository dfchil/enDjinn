#include <dc/fmath.h>
#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>
#include <math.h>

static const alignas(32) uint8_t enj_txr_blob[] = {
#embed "../embeds/enj_modes/texture/pal8/enDjinn512.dt"
};
static const alignas(32) uint8_t enj_palette_blob[] = {
#embed "../embeds/enj_modes/texture/pal8/enDjinn512.dt.pal"
};
static enj_texture_info_t figure_texture_info;

static const alignas(32) uint8_t help_txt_raw[] = {
#embed "../embeds/enj_modes/texture/argb1555_vq_tw/info512.dt"
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
    int32_t size_bump;
    pvr_sprite_hdr_t hdr;
    float base_size;
    float center_x;
    float center_y;
} main_data_t;
typedef struct {
    pvr_sprite_hdr_t hdr;
} info_data_t;

typedef struct {
    main_data_t* mode_data;
    float velocity_x;
    float velocity_y;
} slide_data_t;

alignas(32) static enj_mode_t zoom_mode = {.name = "Zoom into"};
alignas(32) static enj_mode_t slide_mode = {
    .name = "Slide out",
};
alignas(32) static info_data_t info_mode_data;
alignas(32) static enj_mode_t info_mode = {.data = &info_mode_data,
                                           .name = "Info Mode"};

static inline void rotate2d(float x, float y, float sin, float cos,
                            float* out_x, float* out_y) {
    *out_x = (x * cos - y * sin) * ENJ_XSCALE;
    *out_y = x * sin + y * cos;
}
void enDjinn_render(void* data) {
    main_data_t* mdata = (main_data_t*)data;
    float cos, sin;
    fsincosr(-((mdata->rotation % 360) * (F_PI / 180.0f)), &sin, &cos);
    float cur_radius = mdata->base_size - mdata->size_bump;
    alignas(32) float corners[4][3] = {
        {-cur_radius, +cur_radius, 1.0f},
        {-cur_radius, -cur_radius, 1.0f},
        {+cur_radius, -cur_radius, 1.0f},
        {+cur_radius, +cur_radius, 1.0f},
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

void info_renderer(void* data) {
    info_data_t* mdata = (info_data_t*)data;

    float min_x =
        (vid_mode->width - help_texture_info.width) * 0.5f * ENJ_XSCALE;
    float max_x = min_x + help_texture_info.width * ENJ_XSCALE;
    float min_y = (vid_mode->height - 440) * 0.5f;
    float max_y = min_y + help_texture_info.height;

    float corners[4][3] = {
        {min_x, max_y, 2.0f},
        {min_x, min_y, 2.0f},
        {max_x, min_y, 2.0f},
        {max_x, max_y, 2.0f},
    };
    enj_draw_sprite(corners, NULL, &mdata->hdr, NULL);
}
static inline void animate(main_data_t* mdata) {
    mdata->size_bump = MAX(0, mdata->size_bump - 1);
    mdata->rotation++;
}

void info_updater(void* data) {
    enj_ctrlr_state_t** ctrls = enj_ctrl_get_states();
    for (int i = 0; i < 4; i++) {
        if (ctrls[i] != NULL) {
            if (ctrls[i]->button.START == ENJ_BUTTON_DOWN_THIS_FRAME) {
                enj_mode_flag_end_current();
            }
        }
    }
    animate(zoom_mode.data);

    enj_render_list_add(PVR_LIST_TR_POLY, enDjinn_render, zoom_mode.data);
    enj_render_list_add(PVR_LIST_PT_POLY, info_renderer, data);
}

void slide_default_direction(enj_mode_t* prev, enj_mode_t* next) {
    slide_data_t* smdata = (slide_data_t*)next->data;
    smdata->velocity_x = 0.0f;
    smdata->velocity_y = +9.5f;
}

void into_main_indicator(enj_mode_t* prev, enj_mode_t* next) {
    main_data_t* mdata = (main_data_t*)next->data;
    mdata->size_bump = 5;
    mdata->center_x = vid_mode->width * ENJ_XSCALE * 0.5f;
    mdata->center_y = vid_mode->height * 0.5f;
}

void zoom_mode_initializer(enj_mode_t* prev, enj_mode_t* next) {
    main_data_t* mdata = (main_data_t*)next->data;
    into_main_indicator(prev, next);
    mdata->size_bump = mdata->base_size;
}

void zoom_mode_updater(void* data) {
    main_data_t* mdata = (main_data_t*)data;
    animate(mdata);
    if (mdata->size_bump <= 0) {
        enj_mode_flag_end_current();
    } else {
        mdata->size_bump--;
    }
    enj_render_list_add(PVR_LIST_TR_POLY, enDjinn_render, data);
}

void slide_mode_updater(void* data) {
    slide_data_t* smdata = (slide_data_t*)data;
    animate(smdata->mode_data);
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
    enj_render_list_add(PVR_LIST_TR_POLY, enDjinn_render, smdata->mode_data);
}

void main_mode_updater(void* data) {
    main_data_t* mdata = (main_data_t*)data;
    animate(mdata);

    enj_ctrlr_state_t** ctrls = enj_ctrl_get_states();
    for (int i = 0; i < 4; i++) {
        if (ctrls[i] != NULL) {
            if (ctrls[i]->button.START == ENJ_BUTTON_DOWN_THIS_FRAME) {
                enj_mode_push(&info_mode);
            }

            if (ABS(ctrls[i]->joyx) + ABS(ctrls[i]->joyy) > 30) {
                mdata->rotation =
                    (2.5f * F_PI +
                     atan2f(ctrls[i]->joyy, -ctrls[i]->joyx + 0.0001f)) *
                    (180.0f / F_PI);
            }
            uint32_t dpad = ENJ_BUTTON_DOWN_THIS_FRAME
                            << 8;  // ie dpad DOWN pressed this frame
            for (int d = 0; d < 4; d++) {
                if ((ctrls[i]->button.raw & dpad) == dpad) {
                    enj_mode_push(
                        &zoom_mode);  // this will run after slide mode finishes
                    enj_mode_push(&slide_mode);
                    slide_data_t* sd = (slide_data_t*)slide_mode.data;
                    sd->velocity_x = sd->velocity_y = 0.0f;
                    switch (d) {
                        case 0:  // UP
                            sd->velocity_y = -7.5f;
                            break;
                        case 1:  // DOWN
                            sd->velocity_y = +7.5f;
                            break;
                        case 2:  // LEFT
                            sd->velocity_x = -7.5f * ENJ_XSCALE;
                            break;
                        case 3:  // RIGHT
                            sd->velocity_x = +7.5f * ENJ_XSCALE;
                            break;
                        default:
                            break;
                    };
                    break;  // only handle one dpad press per frame
                }
                dpad <<= 2;
            }
        }
    }
    enj_render_list_add(PVR_LIST_TR_POLY, enDjinn_render, data);
}

void setup_textures() {
    // load textures from memory blobs
    enj_texture_load_blob(enj_txr_blob, &figure_texture_info);
    enj_texture_bind_palette(&figure_texture_info, 0);
    enj_texture_load_palette_blob(enj_palette_blob,
                                  figure_texture_info.flags.palette_format,
                                  figure_texture_info.flags.palette_position);

    enj_texture_load_blob(help_txt_raw, &help_texture_info);
}

void setup_modes(enj_mode_t* main_mode, slide_data_t* slide_mode_data) {
    // setup info mode
    pvr_sprite_cxt_t i_cxt;
    pvr_sprite_cxt_txr(&i_cxt, PVR_LIST_PT_POLY, help_texture_info.pvrformat,
                       help_texture_info.width, help_texture_info.height,
                       help_texture_info.ptr, PVR_FILTER_NEAREST);
    i_cxt.gen.culling = PVR_CULLING_NONE;
    pvr_sprite_compile(&info_mode_data.hdr, &i_cxt);
    info_mode.data = &info_mode_data;
    info_mode.mode_updater = info_updater;
    info_mode.on_activation_fn = NULL;

    // setup the other modes
    main_data_t* main_mode_data = (main_data_t*)main_mode->data;
    pvr_sprite_cxt_t f_cxt;
    pvr_sprite_cxt_txr(&f_cxt, PVR_LIST_TR_POLY, figure_texture_info.pvrformat,
                       figure_texture_info.width, figure_texture_info.height,
                       figure_texture_info.ptr, PVR_FILTER_BILINEAR);
    f_cxt.gen.culling = PVR_CULLING_NONE;
    f_cxt.gen.specular = PVR_SPECULAR_ENABLE;
    pvr_sprite_compile(&main_mode_data->hdr, &f_cxt);
    main_mode_data->hdr.argb = 0xffffffff;

    zoom_mode.data = main_mode->data;
    zoom_mode.mode_updater = zoom_mode_updater;
    zoom_mode.on_activation_fn = zoom_mode_initializer;

    slide_mode_data->mode_data = main_mode_data;
    slide_mode.data = slide_mode_data;
    slide_mode.mode_updater = slide_mode_updater;
    slide_mode.on_activation_fn = slide_default_direction;

    enj_mode_push(&slide_mode);
    enj_mode_push(main_mode);
    enj_mode_soft_reset_target_set(enj_mode_get_current_index());
    enj_mode_push(&zoom_mode);
    // pushing doesnt trigger pop function, so call it manually
    zoom_mode.on_activation_fn(NULL, &zoom_mode);
}

int main(__unused int argc, __unused char** argv) {
    // initialize enDjinn state with default values
    enj_state_init_defaults();

    // default pattern is START + A + B + X + Y
    // lets make it easier, A is offset 0 in bitfield and START is
    // offset 8<<1 (two bits per button)
    enj_state_soft_reset_set(ENJ_BUTTON_DOWN << (8 << 1) | ENJ_BUTTON_DOWN);

    if (enj_state_startup() != 0) {
        ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
        return -1;
    }

    setup_textures();
    /* setup at enDjinn modes */
    main_data_t main_mode_data = {
        .rotation = 0,
        .size_bump = 0,
        .base_size = (figure_texture_info.width) * 0.42f,
    };
    enj_mode_t main_mode = {
        .name = "Main Mode",
        .mode_updater = main_mode_updater,
        .data = &main_mode_data,
        .on_activation_fn = into_main_indicator,
    };
    slide_data_t slide_mode_data;
    setup_modes(&main_mode, &slide_mode_data);
    enj_state_run();
    return 0;
}
