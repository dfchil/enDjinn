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

// font src: https://dejavu-fonts.github.io/
static const alignas(32) uint8_t deja_font_blob[] = {
#embed "../embeds/enj_writing/fonts/pal4/23/DejaVuSans.enjfont"
};
alignas(32) static enj_font_header_t deja_font_hdr;
alignas(32) static pvr_sprite_hdr_t deja_font_pvr_hdr;

// font src: https://www.fonttr.com/cmunbi-font
static const alignas(32) uint8_t cmunrm_font_blob[] = {
#embed "../embeds/enj_writing/fonts/pal4/36/cmunrm.enjfont"
};
alignas(32) static enj_font_header_t cmunrm_font_hdr;
alignas(32) static pvr_sprite_hdr_t cmunrm_font_pvr_hdr;

// font src:
// https://github.com/zshoals/Dina-Font-TTF-Remastered?tab=readme-ov-file
static const alignas(32) uint8_t dina_font_blob[] = {
#embed "../embeds/enj_writing/fonts/pal4/16/Dina-Regular.enjfont"
};
alignas(32) static enj_font_header_t dina_font_hdr;
alignas(32) static pvr_sprite_hdr_t dina_font_pvr_hdr_TR;
alignas(32) static pvr_sprite_hdr_t dina_font_pvr_hdr_OP;


const char *lorum_ipsum =
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
} main_data_t;

static inline void rotate2d(float x, float y, float sin, float cos,
                            float *out_x, float *out_y) {
  *out_x = (x * cos - y * sin) * ENJ_XSCALE;
  *out_y = x * sin + y * cos;
}

void render_OP(void *data) {
    static pvr_dr_state_t static_dr_state;
    pvr_dr_init(&static_dr_state);

    pvr_sprite_hdr_t *font_hdr_sq =
        (pvr_sprite_hdr_t *)pvr_dr_target((pvr_dr_state_t){0});
    *font_hdr_sq = dina_font_pvr_hdr_OP;
    pvr_dr_commit(font_hdr_sq);
    int fontstartx = 20 * ENJ_XSCALE;
    int fontstarty = vid_mode->height / 2 +100;
    
    for (char c = ' '; c <= '~'; c++) {
      fontstartx +=
          3 + enj_font_render_glyph(c, &dina_font_hdr, fontstartx, fontstarty,
                                    2.0f, &static_dr_state);
      if (fontstartx > vid_mode->width - 40) {
        fontstartx = 20 * ENJ_XSCALE;
        fontstarty += dina_font_hdr.line_height;
      }
    }
    pvr_dr_finish();
}


void render_TR(void *data) {
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

  static pvr_dr_state_t static_dr_state;
  pvr_dr_init(&static_dr_state);
  enj_draw_sprite(corners, &static_dr_state, &mdata->hdr, NULL);

  struct font_hdrs_s {
    pvr_sprite_hdr_t *hdr;
    enj_font_header_t *font;
  };

  struct font_hdrs_s font_hdrs[] = {
      {&cmunrm_font_pvr_hdr, &cmunrm_font_hdr},
      {&deja_font_pvr_hdr, &deja_font_hdr},
      {&dina_font_pvr_hdr_TR, &dina_font_hdr},
  };

  int fontstartx = 20 * ENJ_XSCALE;
  int fontstarty = 8;
  for (int i = 0; i < 3; i++) {
    pvr_sprite_hdr_t *font_hdr_sq =
        (pvr_sprite_hdr_t *)pvr_dr_target((pvr_dr_state_t){0});
    *font_hdr_sq = *(font_hdrs[i].hdr);
    pvr_dr_commit(font_hdr_sq);

    fontstartx = 20 * ENJ_XSCALE;
    for (char c = ' '; c <= '~'; c++) {
      fontstartx +=
          3 + enj_font_render_glyph(c, font_hdrs[i].font, fontstartx, fontstarty,
                                    2.0f, &static_dr_state);
      if (fontstartx > vid_mode->width - 40) {
        fontstartx = 20 * ENJ_XSCALE;
        fontstarty += font_hdrs[i].font->line_height;
      }
    }
    fontstarty += font_hdrs[i].font->line_height << 1;
  }

  char *li = lorum_ipsum;
  fontstartx = 20 * ENJ_XSCALE;
  fontstarty += font_hdrs[0].font->line_height << 1;
  while (*li != '\0') {
    fontstartx += 1 + enj_font_render_glyph(*li, font_hdrs[2].font, fontstartx,
                                            fontstarty, 2.0f, &static_dr_state);
    if (fontstartx > vid_mode->width - 40) {
      fontstartx = 20 * ENJ_XSCALE;
      fontstarty += font_hdrs[2].font->line_height;
    }
    li++;
  }
  pvr_dr_finish();
}
void main_mode_updater(void *data) {
  main_data_t *mdata = (main_data_t *)data;
  mdata->rotation++;
  enj_renderlist_add(PVR_LIST_TR_POLY, render_TR, data);
  enj_renderlist_add(PVR_LIST_OP_POLY, render_OP, data);
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

void setup_fonts() {
  if (!enj_font_from_blob(cmunrm_font_blob, &cmunrm_font_hdr)) {
    ENJ_DEBUG_PRINT("Failed to load cmunrm_font_hdr from blob\n");
    return;
  }
  if (!enj_font_TR_header(&cmunrm_font_hdr, &cmunrm_font_pvr_hdr, 1,
                          (enj_color_t){.raw = 0xffffffff}, PVR_PAL_ARGB8888)) {
    ENJ_DEBUG_PRINT("Failed to setup cmunrm_font_hdr header\n");
    return;
  }
  if (!enj_font_from_blob(deja_font_blob, &deja_font_hdr)) {
    ENJ_DEBUG_PRINT("Failed to load deja_font_hdr from blob\n");
    return;
  }
  if (!enj_font_TR_header(&deja_font_hdr, &deja_font_pvr_hdr, 1,
                          (enj_color_t){.raw = 0xffffffff}, PVR_PAL_ARGB8888)) {
    ENJ_DEBUG_PRINT("Failed to setup deja_font_hdr header\n");
    return;
  }

  if (!enj_font_from_blob(dina_font_blob, &dina_font_hdr)) {
    ENJ_DEBUG_PRINT("Failed to load dina_font_hdr from blob\n");
    return;
  }
  if (!enj_font_TR_header(&dina_font_hdr, &dina_font_pvr_hdr_TR, 1,
                          (enj_color_t){.raw = 0xffffffff}, PVR_PAL_ARGB8888)) {
    ENJ_DEBUG_PRINT("Failed to setup dina_font_hdr header\n");
    return;
  }
  if (!enj_font_OP_header(&dina_font_hdr, &dina_font_pvr_hdr_OP, 2,
                          (enj_color_t){.raw = 0xffffffff},
                          (enj_color_t){.raw = 0xff000000}, PVR_PAL_ARGB8888)) {
    ENJ_DEBUG_PRINT("Failed to setup dina_font_hdr header\n");
    return;
  }
}

int main(__unused int argc, __unused char **argv) {
  // initialize enDjinn state with default values
  enj_state_defaults();
  // default soft-reset pattern is START + A + B + X + Y.
  // Lets make it easier with just START + A.
  // A is offset 0 in bitfield and START is offset
  // 8<<1 (two bits per button)
  enj_state_set_soft_reset(BUTTON_DOWN << (8 << 1) | BUTTON_DOWN);
  enj_state_get()->video.bg_color.raw = (enj_color_t){.raw = 0xFF00bbff}.raw;
  printf("bg color set to 0x%x \n", (void *)enj_state_get()->video.bg_color.raw);

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
  setup_fonts();
  setup_modes(&main_mode);
  enj_mode_push(&main_mode);
  enj_run();
  enj_texture_unload(&figure_texture_info);
  return 0;
}
