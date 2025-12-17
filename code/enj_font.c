#include <enDjinn/enj_defs.h>
#include <enDjinn/enj_draw.h>
#include <enDjinn/enj_font.h>
#include <enDjinn/enj_bitmap.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float enj_font_zvalue = 10.0f;
static uint8_t enj_font_scale = 1;
static uint8_t enj_font_letter_spacing = 2;

void enj_font_set_scale(uint8_t scale) {
  if (scale == 0) {
    ENJ_DEBUG_PRINT("Scale cannot be zero, ignoring\n");
    return;
  }
  enj_font_scale = scale;
}

void enj_font_set_zvalue(float zvalue) { enj_font_zvalue = zvalue; }

void enj_font_set_letter_spacing(uint8_t spacing) {
  enj_font_letter_spacing = spacing;
}

int enj_font_from_blob(const uint8_t *blob, enj_font_header_t *out_font) {
  memcpy(out_font, blob, sizeof(enj_font_header_t));
  int height = 1 << out_font->log2height;
  int width = 1 << out_font->log2width;

  size_t pvr_mem_size = ((width * height) >> 1);
  pvr_ptr_t pvr_data = pvr_mem_malloc(pvr_mem_size);
  if (!pvr_data) {
    printf("Error allocating memory for font PVR data\n");
    return 0;
  }
  pvr_txr_load_ex(blob + sizeof(enj_font_header_t), pvr_data,
                  1 << out_font->log2width, 1 << out_font->log2height,
                  PVR_TXRLOAD_4BPP);
  out_font->pvr_data = (uint32_t)pvr_data;

  return 1;
}

int enj_font_from_file(const char *path, enj_font_header_t *out_font) {
  int success = 1;
  FILE *file = NULL;
  do {
    file = fopen(path, "rb");
    if (!file) {
      printf("Error opening font file %s: %s\n", path, strerror(errno));
      success = 0;
      break;
    }

    if (fread(out_font, sizeof(enj_font_header_t), 1, file) != 1) {
      printf("Error reading font header from file %s\n", path);
      success = 0;
      break;
    }
    size_t blobsize =
        sizeof(enj_font_header_t) +
        (((1 << (out_font->log2width) * (1 << out_font->log2height))) >> 1);
    uint8_t *font_blob = memalign(32, blobsize);
    if (!font_blob) {
      printf("Error allocating memory for font blob from file %s\n", path);
      success = 0;
      break;
    }
    memcpy(font_blob, out_font, sizeof(enj_font_header_t));
    if (fread(font_blob + sizeof(enj_font_header_t),
              blobsize - sizeof(enj_font_header_t), 1, file) != 1) {
      printf("Error reading font blob from file %s\n", path);
      free(font_blob);
      success = 0;
      break;
    }
    success = enj_font_from_blob(font_blob, out_font);
    free(font_blob);
  } while (0);

  if (file != NULL) {
    fclose(file);
  }
  return success;
}

int enj_font_PAL_TR_header(enj_font_header_t *font, pvr_sprite_hdr_t *hdr,
                           uint8_t palette_entry, enj_color_t front_color,
                           pvr_palfmt_t pal_fmt) {
  // generate transparent palette
  pvr_set_pal_format(pal_fmt);
  uint32_t palette_offset = palette_entry
                            << (pal_fmt == PVR_PAL_ARGB8888 ? 8 : 4);

  enj_color_t color = {.a = 0, .r = 255, .g = 255, .b = 255};
  for (int i = 0; i < 16; i++) {
    color.a = i * 17; // 0, 17, 34, ..., 255
    pvr_set_pal_entry(palette_offset + i, color.raw);
  }

  // setup header
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_txr(
      &cxt, PVR_LIST_TR_POLY,
      PVR_TXRFMT_PAL4BPP |
          (palette_entry << (pal_fmt == PVR_PAL_ARGB8888 ? 25 : 21)),
      1 << font->log2width, 1 << font->log2height, (pvr_ptr_t)font->pvr_data,
      PVR_FILTER_NEAREST);
  pvr_sprite_compile(hdr, &cxt);
  hdr->argb = front_color.raw;

  return 1;
}

static inline void palette_color_mixer(enj_color_t front_color,
                                       enj_color_t back_color,
                                       uint8_t palette_entry,
                                       pvr_palfmt_t pal_fmt) {
  int dr = (front_color.r - back_color.r) / 15;
  int dg = (front_color.g - back_color.g) / 15;
  int db = (front_color.b - back_color.b) / 15;

  uint32_t palette_offset = palette_entry
                            << (pal_fmt == PVR_PAL_ARGB8888 ? 8 : 4);
  for (int i = 0; i < 16; i++) {
    enj_color_t color = {.a = 255,
                         .r = back_color.r + dr * i,
                         .g = back_color.g + dg * i,
                         .b = back_color.b + db * i};
    pvr_set_pal_entry(palette_offset + i, color.raw);
  }
}

int enj_font_PAL_OP_header(enj_font_header_t *font, pvr_sprite_hdr_t *hdr,
                           uint8_t palette_entry, enj_color_t front_color,
                           enj_color_t back_color, pvr_palfmt_t pal_fmt) {
  // generate opaque palette
  palette_color_mixer(front_color, back_color, palette_entry, pal_fmt);

  // setup header
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_txr(&cxt, PVR_LIST_OP_POLY,
                     PVR_TXRFMT_PAL4BPP | (palette_entry << 25),
                     1 << font->log2width, 1 << font->log2height,
                     (pvr_ptr_t)(uintptr_t)font->pvr_data, PVR_FILTER_NEAREST);
  pvr_sprite_compile(hdr, &cxt);
  hdr->argb = front_color.raw;

  return 1;
}

int enj_font_PAL_PT_header(enj_font_header_t *font, pvr_sprite_hdr_t *hdr,
                           uint8_t palette_entry, enj_color_t front_color,
                           enj_color_t back_color, pvr_palfmt_t pal_fmt) {
  // generate punchthrough palette
  palette_color_mixer(front_color, back_color, palette_entry, pal_fmt);
  pvr_set_pal_entry(palette_entry << (pal_fmt == PVR_PAL_ARGB8888 ? 8 : 4), 0);

  // setup header
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_txr(&cxt, PVR_LIST_PT_POLY,
                     PVR_TXRFMT_PAL4BPP | (palette_entry << 25),
                     1 << font->log2width, 1 << font->log2height,
                     (pvr_ptr_t)(uintptr_t)font->pvr_data, PVR_FILTER_NEAREST);
  pvr_sprite_compile(hdr, &cxt);
  hdr->argb = front_color.raw;

  return 1;
}

int enj_font_glyph_uv_coords(enj_font_header_t *font, char glyph, uint32_t *auv,
                             uint32_t *buv, uint32_t *cuv) {
  if (glyph > '~' || glyph < '!') {
    // out of range
    printf("Glyph '%c' out of range for font\n", glyph);
    return 0;
  }
  if (glyph == ' ') {
    *auv = PVR_PACK_16BIT_UV(0.999f, 0.999f);
    *buv = PVR_PACK_16BIT_UV(1.0f, 0.999f);
    *cuv = PVR_PACK_16BIT_UV(1.0f, 1.0f);
    return 1;
  }

  int glyph_index = (uint32_t)glyph - '!';
  enj_glyph_offset_t glyph_start = font->glyph_endings[glyph_index];
  enj_glyph_offset_t glyph_end = font->glyph_endings[glyph_index + 1];

  float startx = glyph_start.x_min > glyph_end.x_min ? 0 : glyph_start.x_min;

  // uint32_t glyph_width = glyph_end.x_min - glyph_start;
  float inv_txr_width = 1.0f / (1 << font->log2width);
  float inv_txr_height = 1.0f / (1 << font->log2height);

  *auv = PVR_PACK_16BIT_UV(startx * inv_txr_width,
                           (float)(font->line_height * (glyph_start.line + 1)) *
                               inv_txr_height);

  *buv = PVR_PACK_16BIT_UV(startx * inv_txr_width,
                           (float)(font->line_height * glyph_start.line) *
                               inv_txr_height);
  *cuv = PVR_PACK_16BIT_UV((float)(glyph_end.x_min) * inv_txr_width,
                           (float)(font->line_height * glyph_start.line) *
                               inv_txr_height);

  return 1;
}

static inline int enj_font_space_width(enj_font_header_t *font) {
  return ((font->line_height * enj_font_scale) >> 2);
}

int enj_font_render_glyph(char glyph, enj_font_header_t *font, int16_t x,
                          int16_t y, pvr_dr_state_t *state_ptr) {
  if (glyph < ' ' || glyph > '~') {
    ENJ_DEBUG_PRINT("Glyph '%c' out of range for font\n", glyph);
    return -1;
  }
  int glyph_index = (uint32_t)glyph - '!';
  enj_glyph_offset_t glyph_start = font->glyph_endings[glyph_index];
  if (glyph == ' ' || !glyph_start.available) {
    return enj_font_space_width(font);
  }
  enj_glyph_offset_t glyph_end = font->glyph_endings[glyph_index + 1];
  int startx = glyph_start.x_min > glyph_end.x_min ? 0 : glyph_start.x_min;
  int width = (glyph_end.x_min - startx) * enj_font_scale;

  float min_x = x * ENJ_XSCALE;
  float max_x = (x + width) * ENJ_XSCALE;
  float max_y = y + (font->line_height * enj_font_scale);

  float corners[4][3] = {{min_x, max_y, enj_font_zvalue},
                         {min_x, (float)y, enj_font_zvalue},
                         {max_x, (float)y, enj_font_zvalue},
                         {max_x, max_y, enj_font_zvalue}};
  uint32_t texcoords[3];
  enj_font_glyph_uv_coords(font, glyph, &texcoords[0], &texcoords[1],
                           &texcoords[2]);
  enj_draw_sprite(corners, state_ptr, NULL, texcoords);
  return width;
}

int enj_font_string_width(const char *text, enj_font_header_t *font) {
  int output = 0;
  while (*text != '\0') {
    if (*text < ' ' || *text > '~') {
      ENJ_DEBUG_PRINT("Glyph '%c' out of range for font\n", *text);
      continue;
    }
    int glyph_index = (uint32_t)(*text) - '!';
    if (*text == ' ' || !(font->glyph_endings[glyph_index].available)) {
      output += enj_font_space_width(font);
    } else if (*text >= '!' && *text <= '~') {
      enj_glyph_offset_t glyph_start = font->glyph_endings[glyph_index];
      enj_glyph_offset_t glyph_end = font->glyph_endings[glyph_index + 1];
      int startx = glyph_start.x_min > glyph_end.x_min ? 0 : glyph_start.x_min;
      int width = (glyph_end.x_min - startx) * enj_font_scale;
      output += width;
    }
    output += enj_font_letter_spacing * enj_font_scale;
    text++;
  }
  return output;
}

int enj_font_string_render(const char *text, enj_font_header_t *font,
                           int16_t x, int16_t y,
                           pvr_sprite_hdr_t *sprite_header,
                           pvr_dr_state_t *state_ptr) {
  static pvr_dr_state_t static_dr_state;
  if (state_ptr == NULL) {
    pvr_dr_init(&static_dr_state);
    state_ptr = &static_dr_state;
  }
  if (sprite_header != NULL) {
    pvr_sprite_hdr_t *hdr_ptr = (pvr_sprite_hdr_t *)pvr_dr_target(*state_ptr);
    *hdr_ptr = *sprite_header;
    pvr_dr_commit(hdr_ptr);
  }

  int x_pos = x;
  while (*text != '\0') {
    x_pos += (enj_font_letter_spacing * enj_font_scale) +
             enj_font_render_glyph(*text, font, x_pos, y, state_ptr);
    text++;
  }
  if (state_ptr == &static_dr_state) {
    pvr_dr_finish();
  }
  return x_pos - x;
}


static inline uint8_t extr_4bpp_pixel(uint8_t *data_4bpp, int index) {
  uint8_t byte = data_4bpp[index >> 1];
  if (index & 1) {
    return (byte >> 4) & 0x0F;
  } else {
    return byte & 0x0F;
  }
}

pvr_ptr_t enj_font_to_16bit_texture(enj_font_header_t *font, uint8_t *data_4bpp,
                                    pvr_pixel_mode_t mode,
                                    enj_color_t front_color,
                                    enj_color_t back_color) {
  if (mode == PVR_PIXEL_MODE_PAL_4BPP || mode == PVR_PIXEL_MODE_PAL_8BPP) {
    ENJ_DEBUG_PRINT("cannot convert to paletted texture\n");
    return NULL;
  }

  int height = 1 << font->log2height;
  int width = 1 << font->log2width;
  size_t pvr_mem_size = width * height * sizeof(uint16_t);
  pvr_ptr_t pvr_data = pvr_mem_malloc(pvr_mem_size);
  if (!pvr_data) {
    printf("Error allocating memory for font PVR data\n");
    return NULL;
  }

  uint16_t buffer[width * height * sizeof(uint16_t)];
  int dr, dg, db;

  switch (mode) {
  case PVR_PIXEL_MODE_ARGB4444:
    for (int i = 0; i < width * height; i++) {
      buffer[i] = 0xffffff00 | extr_4bpp_pixel(data_4bpp, i);
    }
    break;
  case PVR_PIXEL_MODE_ARGB1555:
    dr = (front_color.r - back_color.r) / 15;
    dg = (front_color.g - back_color.g) / 15;
    db = (front_color.b - back_color.b) / 15;

    if (font->palette_type == ENJ_FONT_4BIT_PALETTE){
        for (int i = 0; i < width * height; i++) {
          uint8_t pixel_4bpp = extr_4bpp_pixel(data_4bpp, i);
          buffer[i] = (pixel_4bpp > 0) << 15 | 0x7fff;
    
          // (((back_color.r + dr) * pixel_4bpp) & 0x1F) << 6 |
          //             (((back_color.g + dg) * pixel_4bpp) & 0x1F) << 1
          //             |
          //             (((back_color.b + db) * pixel_4bpp) & 0x1F) >> 3;
          // buffer[i] = 0x7FE0;
        }
    } else {
        enj_bitmap_t bmap;
        bmap.width = width;
        bmap.height = height;
        bmap.data = data_4bpp;
        for (int i = 0; i < width * height; i++) {
            buffer[i] = enj_bitmap_get(&bmap, i % width, i / width) << 15 | 0x7fff;
        }
    }
    break;
  case PVR_PIXEL_MODE_RGB565:
    dr = (front_color.r - back_color.r) / 15;
    dg = (front_color.g - back_color.g) / 15;
    db = (front_color.b - back_color.b) / 15;
    for (int i = 0; i < width * height; i++) {
      uint8_t pixel_4bpp = extr_4bpp_pixel(data_4bpp, i);
      buffer[i] = ((back_color.r + dr * pixel_4bpp) & 0xF8) << 8 |
                  ((back_color.g + dg * pixel_4bpp) & 0xFC) << 3 |
                  ((back_color.b + db * pixel_4bpp) & 0xF8) >> 3;
    }
    break;
  default:
    return NULL;
  }
  pvr_txr_load_ex((uint8_t *)buffer, pvr_data, width, height,
                  PVR_TXRLOAD_16BPP);
  return pvr_data;
}
