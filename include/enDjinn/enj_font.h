#ifndef ENJ_FONTS_H
#define ENJ_FONTS_H

#include <dc/pvr.h>
#include <enDjinn/enj_types.h>

typedef struct {
  uint16_t line : 4;
  uint16_t offset : 11;
  uint16_t available : 1;
} enj_glyph_offset_t;

typedef struct {
  struct {
    uint8_t log2width : 4;
    uint8_t log2height : 4;
  };
  uint8_t line_height;
  /* from '!' to '~', last +1 is padding */
  enj_glyph_offset_t glyph_starts[-33 + 126 + 1 + 1];
} enj_font_header_t;

int enj_font_glyph_uv_coords(enj_font_header_t *font, char glyph, float *u0,
                             float *v0, float *u1, float *v1);

/** Configure a PVR sprite header for rendering text glyphs
 * @param font Pointer to font header
 * @param hdr Pointer to PVR sprite header to configure
 * @param palette_offset Palette offset to use for the font
 * @param front_color Color to use for the front of the glyphs
 * @return 1 on success, 0 on failure
 */
 int enj_font_TR_header(enj_font_header_t *font, pvr_sprite_hdr_t *hdr,
                       uint8_t palette_offset, enj_color_t front_color);


/** Configure a PVR sprite header for rendering text glyphs with outline
 * @param font Pointer to font header
 * @param hdr Pointer to PVR sprite header to configure
 * @param palette_offset Palette offset to use for the font
 * @param front_color Color to use for as glyph fill
 * @param back_color Color to blend towards for the outline
 * @return 1 on success, 0 on failure
 */
int enj_font_OP_header(enj_font_header_t *font, pvr_sprite_hdr_t *hdr,
                       uint8_t palette_offset, enj_color_t front_color,
                       enj_color_t back_color);


/** Configure a PVR sprite header for rendering text glyphs with drop shadow
 * @param font Pointer to font header
 * @param hdr Pointer to PVR sprite header to configure
 * @param palette_offset Palette offset to use for the font
 * @param front_color Color to use for as glyph fill
 * @param back_color Color to blend towards for the outline
 * @return 1 on success, 0 on failure
 * 
int enj_font_PT_header(enj_font_header_t *font, pvr_sprite_hdr_t *hdr,
                       uint8_t palette_offset, enj_color_t front_color,
                       enj_color_t back_color);

/** Render a single sprite glyph to the PVR command buffer
 * @param glyph Character to draw
 * @param font Pointer to font header
 * @param hdr Pointer to pre-configured PVR sprite header
 * @param x X position to draw at in pixels
 * @param y Y position to draw at in pixels
 * 
 * @return width of rendered glyph in pixels
 * 
 */
int enj_font_render_glyph(char glyph, enj_font_header_t *font,
                          pvr_sprite_hdr_t *hdr, uint16_t x, uint16_t y);

/**
 * Render a text string to the PVR command buffer
 * @param text Null-terminated string to draw
 * @param font Pointer to font header
 * @param hdr Pointer to pre-configured PVR sprite header
 * @param x X position to draw at in pixels
 * @param y Y position to draw at in pixels
 * @return width of rendered text in pixels
 */
int enj_font_render_text(const char *text, enj_font_header_t *font,
                         pvr_sprite_hdr_t *hdr, uint16_t x, uint16_t y);

/** Calculate the width of a text string in pixels
 * @param text Null-terminated string to measure
 * @param font Pointer to font header
 * @return width of text in pixels
 */
int enj_font_text_width(const char *text, enj_font_header_t *font);

#endif // ENJ_FONTS_H
