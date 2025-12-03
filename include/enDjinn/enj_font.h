#ifndef ENJ_FONTS_H
#define ENJ_FONTS_H

#include <dc/pvr.h>
#include <enDjinn/enj_font_types.h>
#include <enDjinn/enj_types.h>

/** Load an enDjinn font from a file
 * @param path Path to font file
 * @param out_font Pointer to store loaded font header
 * @return 1 on success, 0 on failure
 */
int enj_font_load(const char* path, enj_font_header_t* out_font);

/** Configure a PVR sprite header for rendering text glyphs
 * @param font Pointer to font header
 * @param hdr Pointer to PVR sprite header to configure
 * @param palette_offset Palette offset to use for the font
 * @param front_color Color to use for the front of the glyphs
 * @return 1 on success, 0 on failure
 */
int enj_font_TR_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                       uint8_t palette_offset, enj_color_t front_color);

/** Configure a PVR sprite header for rendering text glyphs with outline
 * @param font Pointer to font header
 * @param hdr Pointer to PVR sprite header to configure
 * @param palette_offset Palette offset to use for the font
 * @param front_color Color to use for as glyph fill
 * @param back_color Color to blend towards for the outline
 * @return 1 on success, 0 on failure
 */
int enj_font_OP_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                       uint8_t palette_offset, enj_color_t front_color,
                       enj_color_t back_color);

/** Configure a PVR sprite header for rendering text glyphs with drop shadow
 * @param font Pointer to font header
 * @param hdr Pointer to PVR sprite header to configure
 * @param palette_offset Palette offset to use for the font
 * @param front_color Color to use for as glyph fill
 * @param back_color Color to blend towards for the outline
 * @return 1 on success, 0 on failure
 */
int enj_font_PT_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                       uint8_t palette_offset, enj_color_t front_color,
                       enj_color_t back_color);

/** Get UV coordinates for a glyph in a font
 * @param font Pointer to font header
 * @param glyph Character to get UVs for
 * @param u0 Pointer to store U0 coordinate
 * @param v0 Pointer to store V0 coordinate
 * @param u1 Pointer to store U1 coordinate
 * @param v1 Pointer to store V1 coordinate
 * @return 1 on success, 0 on failure
 */
int enj_font_glyph_uv_coords(enj_font_header_t* font, char glyph, float* u0,
                             float* v0, float* u1, float* v1);

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
int enj_font_render_glyph(char glyph, enj_font_header_t* font,
                          pvr_sprite_hdr_t* hdr, uint16_t x, uint16_t y);

/**
 * Render a text string to the PVR command buffer
 * @param text Null-terminated string to draw
 * @param font Pointer to font header
 * @param hdr Pointer to pre-configured PVR sprite header
 * @param x X position to draw at in pixels
 * @param y Y position to draw at in pixels
 * @return width of rendered text in pixels
 */
int enj_font_render_text(const char* text, enj_font_header_t* font,
                         pvr_sprite_hdr_t* hdr, uint16_t x, uint16_t y);

/** Render text within a bounding box
 * @param text Null-terminated string to draw
 * @param font Pointer to font header
 * @param hdr Pointer to pre-configured PVR sprite header
 * @param x X position of top-left corner of box in pixels
 * @param y Y position of top-left corner of box in pixels
 * @param box_width Width of bounding box in pixels
 * @param box_height Height of bounding box in pixels
 * 
 * @return number of lines rendered
 */
int enj_font_render_text_in_box(const char* text, enj_font_header_t* font,
                                 pvr_sprite_hdr_t* hdr, uint16_t x, uint16_t y,
                                 uint16_t box_width, uint16_t box_height);

/** Calculate the width of a text string in pixels
 * @param text Null-terminated string to measure
 * @param font Pointer to font header
 * @return width of text in pixels
 */
int enj_font_text_width(const char* text, enj_font_header_t* font);

#endif  // ENJ_FONTS_H
