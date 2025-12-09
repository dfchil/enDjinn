#include <dc/fmath.h>
#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>
#include <math.h>

static const alignas(32) uint8_t enj_txr_blob[] = {
#embed "../embeds/enj_writing/texture/pal8/enDjinn512.dt"
};
static const alignas(32) uint8_t enj_palette_blob[] = {
#embed "../embeds/enj_writing/texture/pal8/enDjinn512.dt.pal"
};
static enj_texture_info_t figure_texture_info;

typedef struct {
  union {
    struct {
      struct {
        enj_font_header_t *font_hdr;
        pvr_sprite_hdr_t sprite_hdr;
      } dina_16;
      struct {
        enj_font_header_t *font_hdr;
        pvr_sprite_hdr_t sprite_hdr;
      } deja_23;
      struct {
        enj_font_header_t *font_hdr;
        pvr_sprite_hdr_t sprite_hdr;
      } cmunrm_36;
    } named;
    struct {
      enj_font_header_t *font_hdr;
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
  pvr_sprite_hdr_t *hdr;
  enj_font_header_t *font;
};

static const char *lorem_ipsum =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
    "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
    "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
    "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
    "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
    "mollit anim id est laborum.";

typedef struct {
  uint32_t rotation;
  pvr_sprite_hdr_t hdr;
  float base_size;
  float center_x;
  float center_y;
} mode_data_t;

static inline void rotate2d(float x, float y, float sin, float cos,
                            float *out_x, float *out_y) {
  *out_x = (x * cos - y * sin) * ENJ_XSCALE;
  *out_y = x * sin + y * cos;
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
  mode_data_t *main_mode_data = (mode_data_t *)main_mode->data;
  pvr_sprite_cxt_t f_cxt;
  pvr_sprite_cxt_txr(&f_cxt, PVR_LIST_TR_POLY, figure_texture_info.pvrformat,
                     figure_texture_info.width, figure_texture_info.height,
                     figure_texture_info.ptr, PVR_FILTER_BILINEAR);
  pvr_sprite_compile(&main_mode_data->hdr, &f_cxt);
  main_mode_data->hdr.argb = 0xffffffff;
}

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
  enj_font_header_t *font_hdrs_ql[] = {
      &dina_font_16_hdr,
      &deja_23_font_hdr,
      &cmunrm_36_font_hdr,
  };
  for (int i = 0; i < 3; i++) {
    fonts_OP.indexed[i].font_hdr = font_hdrs_ql[i];
    fonts_TR.indexed[i].font_hdr = font_hdrs_ql[i];
    fonts_PT.indexed[i].font_hdr = font_hdrs_ql[i];
    if (!enj_font_PAL_TR_header(
            fonts_TR.indexed[i].font_hdr, &fonts_TR.indexed[i].sprite_hdr, 1,
            (enj_color_t){.raw = 0xffffc010}, PVR_PAL_ARGB8888)) {
      ENJ_DEBUG_PRINT("Failed to setup font_hdrs_TR[%d] header\n", i);
      return;
    }
    if (!enj_font_PAL_OP_header(
            fonts_OP.indexed[i].font_hdr, &fonts_OP.indexed[i].sprite_hdr, 2,
            (enj_color_t){.raw = 0xf171717f},
            (enj_color_t){.raw = enj_state_get()->video.bg_color.raw},
            PVR_PAL_ARGB8888)) {
      ENJ_DEBUG_PRINT("Failed to setup font_hdrs_OP[%d] header\n", i);
      return;
    }
    if (!enj_font_PAL_PT_header(
            fonts_PT.indexed[i].font_hdr, &fonts_PT.indexed[i].sprite_hdr, 3,
            (enj_color_t){.raw = 0xffff00ff},
            (enj_color_t){.raw = enj_state_get()->video.bg_color.raw},
            PVR_PAL_ARGB8888)) {
      ENJ_DEBUG_PRINT("Failed to setup font_hdrs_PT[%d] header\n", i);
      return;
    }
  }
}

void render_OP(void *__unused) {
  static pvr_dr_state_t static_dr_state;
  pvr_dr_init(&static_dr_state);

  pvr_sprite_hdr_t *font_hdr_sq =
      (pvr_sprite_hdr_t *)pvr_dr_target((pvr_dr_state_t){0});
  *font_hdr_sq = fonts_OP.named.cmunrm_36.sprite_hdr;
  pvr_dr_commit(font_hdr_sq);
  int fontstartx = 20 * ENJ_XSCALE;
  int fontstarty = 0;

  const char *li = lorem_ipsum;
  while (*li != '\0') {
    fontstartx += 1 + enj_font_render_glyph(
                          *li, fonts_OP.named.cmunrm_36.font_hdr, fontstartx,
                          fontstarty, 0.5f, &static_dr_state);
    if (fontstartx > vid_mode->width - 40) {
      fontstartx = 20 * ENJ_XSCALE;
      fontstarty += fonts_OP.named.cmunrm_36.font_hdr->line_height;
    }
    li++;
  }
  pvr_dr_finish();
}

void render_PT(void *data) {
  static pvr_dr_state_t static_dr_state;
  pvr_dr_init(&static_dr_state);

  int fontstartx = 20 * ENJ_XSCALE;
  int fontstarty = 0;
  for (int i = 0; i < 3; i++) {
    pvr_sprite_hdr_t *font_hdr_sq =
        (pvr_sprite_hdr_t *)pvr_dr_target((pvr_dr_state_t){0});
    *font_hdr_sq = fonts_PT.indexed[i].sprite_hdr;
    pvr_dr_commit(font_hdr_sq);

    fontstartx = 20 * ENJ_XSCALE;
    for (char c = ' '; c <= '~'; c++) {
      fontstartx +=
          3 + enj_font_render_glyph(c, fonts_PT.indexed[i].font_hdr, fontstartx,
                                    fontstarty, 2.0f, &static_dr_state);
      if (fontstartx > vid_mode->width - 40) {
        fontstartx = 20 * ENJ_XSCALE;
        fontstarty += fonts_PT.indexed[i].font_hdr->line_height;
      }
    }
    fontstarty += fonts_PT.indexed[i].font_hdr->line_height << 1;
  }
  pvr_dr_finish();
}

void render_TR(void *data) {
  mode_data_t *mdata = (mode_data_t *)data;
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

  static pvr_dr_state_t static_dr_state;
  pvr_dr_init(&static_dr_state);
  enj_draw_sprite(corners, &static_dr_state, &mdata->hdr, NULL);

  int fontstartx = 20 * ENJ_XSCALE;
  int fontstarty = (vid_mode->height >> 1) + 20;
  for (int i = 1; i < 3; i++) {
    pvr_sprite_hdr_t *font_hdr_sq =
        (pvr_sprite_hdr_t *)pvr_dr_target((pvr_dr_state_t){0});
    *font_hdr_sq = fonts_TR.indexed[i].sprite_hdr;
    pvr_dr_commit(font_hdr_sq);

    fontstartx = 20 * ENJ_XSCALE;
    for (char c = ' '; c <= '~'; c++) {
      fontstartx +=
          3 + enj_font_render_glyph(c, fonts_TR.indexed[i].font_hdr, fontstartx,
                                    fontstarty, 2.0f, &static_dr_state);
      if (fontstartx > vid_mode->width - 40) {
        fontstartx = 20 * ENJ_XSCALE;
        fontstarty += fonts_TR.indexed[i].font_hdr->line_height;
      }
    }
    fontstarty += fonts_TR.indexed[i].font_hdr->line_height << 1;
  }
  pvr_dr_finish();
}

void main_mode_updater(void *data) {
  mode_data_t *mdata = (mode_data_t *)data;
  mdata->rotation++;
  enj_renderlist_add(PVR_LIST_OP_POLY, render_OP, data);
  enj_renderlist_add(PVR_LIST_TR_POLY, render_TR, data);
  enj_renderlist_add(PVR_LIST_PT_POLY, render_PT, data);
}

int main(__unused int argc, __unused char **argv) {

  // initialize enDjinn state with default values
  enj_state_defaults();
  // default soft-reset pattern is START + A + B + X + Y.
  // Lets make it easier with just START + A.
  // A is offset 0 in bitfield and START is offset
  // 8<<1 (two bits per button)
  enj_state_set_soft_reset(BUTTON_DOWN << (8 << 1) | BUTTON_DOWN);
  // enj_state_get()->video.bg_color.raw = (enj_color_t){.raw = 0xFF00bbff}.raw;

  if (enj_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }
  setup_textures();
  /* setup at enDjinn modes */
  mode_data_t main_mode_data = {
      .rotation = 0,
      .base_size = (figure_texture_info.width) * 0.63f,
      .center_x = vid_mode->width * ENJ_XSCALE * 0.5f,
      .center_y = vid_mode->height * 0.5f,
  };
  enj_mode_t main_mode = {
      .name = "Main Mode",
      .mode_updater = main_mode_updater,
      .data = &main_mode_data,
  };
  setup_fonts();
  setup_modes(&main_mode);
  enj_mode_push(&main_mode);
  enj_run();
  enj_texture_unload(&figure_texture_info);
  return 0;
}