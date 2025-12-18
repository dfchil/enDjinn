#include <dc/fmath.h>
#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>
#include <math.h>
typedef struct {
    union {
        struct {
            struct {
                enj_font_header_t* font_hdr;
                pvr_sprite_hdr_t sprite_hdr;
            } dina_16;
            struct {
                enj_font_header_t* font_hdr;
                pvr_sprite_hdr_t sprite_hdr;
            } deja_23;
            struct {
                enj_font_header_t* font_hdr;
                pvr_sprite_hdr_t sprite_hdr;
            } cmunrm_36;
        } named;
        struct {
            enj_font_header_t* font_hdr;
            pvr_sprite_hdr_t sprite_hdr;
        } indexed[3];
    };
} font_setup_pack_t;

// font src:
// https://github.com/zshoals/Dina-Font-TTF-Remastered?tab=readme-ov-file
static const alignas(32) uint8_t dina_font_blob[] = {
#embed "../embeds/enj_writing/fonts/16/Dina-Regular.enjfont"
};
// font src: https://dejavu-fonts.github.io/
static const alignas(32) uint8_t deja_font_blob[] = {
#embed "../embeds/enj_writing/fonts/23/DejaVuSans.enjfont"
};
// font src: https://www.fonttr.com/cmunbi-font
static const alignas(32) uint8_t cmunrm_font_blob[] = {
#embed "../embeds/enj_writing/fonts/36/cmunrm.enjfont"
};
static alignas(32) enj_font_header_t dina_font_16_hdr;
static alignas(32) enj_font_header_t deja_23_font_hdr;
static alignas(32) enj_font_header_t cmunrm_36_font_hdr;

static alignas(32) font_setup_pack_t fonts_OP = {0};
static alignas(32) font_setup_pack_t fonts_TR = {0};
static alignas(32) font_setup_pack_t fonts_PT = {0};

struct font_hdrs_s {
    pvr_sprite_hdr_t* hdr;
    enj_font_header_t* font;
};

#define MARGIN_LEFT 40
#define MARGIN_RIGHT 40

static const char* lorem_ipsum =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
    "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
    "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
    "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
    "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
    "mollit anim id est laborum.";

typedef struct {
    uint32_t horizontal_scroll;
} mode_data_t;

void setup_fonts() {
    if (!enj_font_from_blob(cmunrm_font_blob, &cmunrm_36_font_hdr)) {
        ENJ_DEBUG_PRINT("Failed to load cmunrm_36_font_hdr from blob\n");
        return;
    }
    if (!enj_font_from_blob(deja_font_blob, &deja_23_font_hdr)) {
        ENJ_DEBUG_PRINT("Failed to load deja_23_font_hdr from blob\n");
        return;
    }
    if (!enj_font_from_blob(dina_font_blob, &dina_font_16_hdr)) {
        ENJ_DEBUG_PRINT("Failed to load dina_font_16_hdr from blob\n");
        return;
    }
    enj_font_header_t* font_hdrs_ql[] = {
        &dina_font_16_hdr,
        &deja_23_font_hdr,
        &cmunrm_36_font_hdr,
    };
    for (int i = 0; i < 3; i++) {
        fonts_OP.indexed[i].font_hdr = font_hdrs_ql[i];
        fonts_TR.indexed[i].font_hdr = font_hdrs_ql[i];
        fonts_PT.indexed[i].font_hdr = font_hdrs_ql[i];
        if (!enj_font_PAL_TR_header(
                fonts_TR.indexed[i].font_hdr, &fonts_TR.indexed[i].sprite_hdr,
                1, (enj_color_t){.raw = 0xffffc010}, PVR_PAL_ARGB8888)) {
            ENJ_DEBUG_PRINT("Failed to setup font_hdrs_TR[%d] header\n", i);
            return;
        }
        if (!enj_font_PAL_OP_header(
                fonts_OP.indexed[i].font_hdr, &fonts_OP.indexed[i].sprite_hdr,
                2, (enj_color_t){.raw = 0xff7f7f7f},
                (enj_color_t){.raw = enj_state_get()->video.bg_color.raw},
                PVR_PAL_ARGB8888)) {
            ENJ_DEBUG_PRINT("Failed to setup font_hdrs_OP[%d] header\n", i);
            return;
        }
        if (!enj_font_PAL_PT_header(
                fonts_PT.indexed[i].font_hdr, &fonts_PT.indexed[i].sprite_hdr,
                3, (enj_color_t){.raw = 0xffff00ff},
                (enj_color_t){.raw = enj_state_get()->video.bg_color.raw},
                PVR_PAL_ARGB8888)) {
            ENJ_DEBUG_PRINT("Failed to setup font_hdrs_PT[%d] header\n", i);
            return;
        }
    }
}

void render_OP(void* __unused) {
    static pvr_dr_state_t static_dr_state;
    pvr_dr_init(&static_dr_state);

    pvr_sprite_hdr_t* font_hdr_sq =
        (pvr_sprite_hdr_t*)pvr_dr_target((pvr_dr_state_t){0});
    *font_hdr_sq = fonts_OP.named.cmunrm_36.sprite_hdr;
    pvr_dr_commit(font_hdr_sq);
    int fontstartx = MARGIN_LEFT;
    int fontstarty = 0;

    enj_font_zvalue_set(0.5f);
    const char* li = lorem_ipsum;
    while (*li != '\0') {
        fontstartx +=
            1 + enj_font_render_glyph(*li, fonts_OP.named.cmunrm_36.font_hdr,
                                      fontstartx, fontstarty, &static_dr_state);
        if (fontstartx > vid_mode->width - MARGIN_RIGHT) {
            fontstartx = MARGIN_LEFT;
            fontstarty += fonts_OP.named.cmunrm_36.font_hdr->line_height;
        }
        li++;
    }
    pvr_dr_finish();
}

void render_PT(void* data) {
    mode_data_t* mdata = (mode_data_t*)data;
    const char* proclamation = "enDjinn makes writing on the Dreamcast easy!";

    enj_font_scale_set(9);
    enj_font_zvalue_set(1.0f);
    int txtwidth = enj_font_string_width(proclamation, enj_qfont_get_header());

    enj_qfont_get_sprite_hdr()->argb = 0xff14a5ff;
    int text_offset = vid_mode->width - ((mdata->horizontal_scroll * ENJ_XSCALE) %
                                         ((vid_mode->width + txtwidth) | 1));

    enj_qfont_write(proclamation, text_offset, 134, PVR_LIST_PT_POLY);
    enj_font_scale_set(1);
    enj_font_zvalue_set(2.0f);

    static pvr_dr_state_t static_dr_state;
    pvr_dr_init(&static_dr_state);

    int fontstartx = MARGIN_LEFT;
    int fontstarty = (vid_mode->height >> 1) - 40;
    enj_font_zvalue_set(1.5f);
    for (int i = 2; i >= 0; i--) {
        pvr_sprite_hdr_t* font_hdr_sq =
            (pvr_sprite_hdr_t*)pvr_dr_target((pvr_dr_state_t){0});
        *font_hdr_sq = fonts_PT.indexed[i].sprite_hdr;
        pvr_dr_commit(font_hdr_sq);

        fontstartx = MARGIN_LEFT;
        for (char c = ' '; c <= '~'; c++) {
            fontstartx += 3 + enj_font_render_glyph(
                                  c, fonts_PT.indexed[i].font_hdr, fontstartx,
                                  fontstarty, &static_dr_state);
            if (fontstartx > vid_mode->width - MARGIN_RIGHT) {
                fontstartx = MARGIN_LEFT;
                fontstarty += fonts_PT.indexed[i].font_hdr->line_height;
            }
        }
        fontstarty += fonts_PT.indexed[i].font_hdr->line_height << 1;
    }

    pvr_dr_finish();
}

void render_TR(void* data) {
    static pvr_dr_state_t static_dr_state;
    pvr_dr_init(&static_dr_state);

    int fontstartx = MARGIN_LEFT;
    int fontstarty = 0;
    enj_font_zvalue_set(2.0f);
    
    for (int i = 1; i < 3; i++) {
        pvr_sprite_hdr_t* font_hdr_sq =
            (pvr_sprite_hdr_t*)pvr_dr_target(static_dr_state);
        *font_hdr_sq = fonts_TR.indexed[i].sprite_hdr;
        pvr_dr_commit(font_hdr_sq);

        fontstartx = MARGIN_LEFT;
        for (char c = ' '; c <= '~'; c++) {
            fontstartx += 3 + enj_font_render_glyph(
                                  c, fonts_TR.indexed[i].font_hdr, fontstartx,
                                  fontstarty, &static_dr_state);
            if (fontstartx > vid_mode->width - MARGIN_RIGHT) {
                fontstartx = MARGIN_LEFT;
                fontstarty += fonts_TR.indexed[i].font_hdr->line_height;
            }
        }
        fontstarty += fonts_TR.indexed[i].font_hdr->line_height << 1;
    }
    pvr_dr_finish();
}

void main_mode_updater(void* data) {
    mode_data_t* mdata = (mode_data_t*)data;
    mdata->horizontal_scroll++;
    enj_render_list_add(PVR_LIST_OP_POLY, render_OP, data);
    enj_render_list_add(PVR_LIST_TR_POLY, render_TR, data);
    enj_render_list_add(PVR_LIST_PT_POLY, render_PT, data);
}

int main(__unused int argc, __unused char** argv) {
    // initialize enDjinn state with default values
    enj_state_init_defaults();
    // default soft-reset pattern is START + A + B + X + Y.
    // Lets make it easier with just START + A.
    // A is offset 0 in bitfield and START is offset
    // 8<<1 (two bits per button)
    enj_state_soft_reset_set(ENJ_BUTTON_DOWN << (8 << 1) | ENJ_BUTTON_DOWN);
    // enj_state_get()->video.bg_color.raw = (enj_color_t){.raw =
    // 0xFF00bbff}.raw;

    if (enj_state_startup() != 0) {
        ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
        return -1;
    }
    /* setup at enDjinn modes */
    mode_data_t main_mode_data = {
        .horizontal_scroll = 0,
    };
    enj_mode_t main_mode = {
        .name = "Main Mode",
        .mode_updater = main_mode_updater,
        .data = &main_mode_data,
    };
    setup_fonts();
    enj_mode_push(&main_mode);
    enj_state_run();
    return 0;
}