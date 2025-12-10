#ifndef ENJ_FONTS_H
#define ENJ_FONTS_H

#include <dc/pvr.h>
#include <enDjinn/enj_font_types.h>
#include <enDjinn/enj_types.h>


/**
 * Set the Z value used for font rendering
 * @param zvalue The Z value to use for rendering
 */
void enj_font_set_zvalue(float zvalue);

/**
 * Load an enDjinn font from a memory blob
 * @param blob Pointer to font data blob
 * @param out_font Pointer to store loaded font header
 * @return 1 on success, 0 on failure
 */
int enj_font_from_blob(const uint8_t* blob, enj_font_header_t* out_font);

/** Load an enDjinn font from a file
 * @param path Path to font file
 * @param out_font Pointer to store loaded font header
 * @return 1 on success, 0 on failure
 */
int enj_font_from_file(const char* path, enj_font_header_t* out_font);

/** Configure a PVR sprite header for rendering text glyphs
 * @param font Pointer to font header
 * @param hdr Pointer to PVR sprite header to configure
 * @param palette_entry Palette offset to use for the font
 * @param front_color Color to use for the front of the glyphs
 * @param pal_fmt Palette format to be used in, 4bit or 8bit
 * @return 1 on success, 0 on failure
 *
 * @note The number of useable palettes depends on the palette format: 64 for
 * 4bit and 4 for 8bit
 */

int enj_font_PAL_TR_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                           uint8_t palette_entry, enj_color_t front_color,
                           pvr_palfmt_t pal_fmt);

/** Configure a PVR sprite header for rendering text glyphs with outline
 * @param font Pointer to font header
 * @param hdr Pointer to PVR sprite header to configure
 * @param palette_entry Palette offset to use for the font
 * @param front_color Color to use for as glyph fill
 * @param back_color Color to blend towards for the outline
 * @param pal_fmt Palette format to be used in, 4bit or 8bit
 * @return 1 on success, 0 on failure
 *
 * @note The number of useable palettes depends on the palette format: 64 for
 4bit and 4 for 8bit

 */
int enj_font_PAL_OP_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                           uint8_t palette_entry, enj_color_t front_color,
                           enj_color_t back_color, pvr_palfmt_t pal_fmt);

/** Configure a PVR sprite header for rendering text glyphs with drop shadow
 * @param font Pointer to font header
 * @param hdr Pointer to PVR sprite header to configure
 * @param palette_entry Palette offset to use for the font
 * @param front_color Color to use for as glyph fill
 * @param back_color Color to blend towards for the outline
 * @param pal_fmt Palette format to be used in, 4bit or 8bit
 * @return 1 on success, 0 on failure
 *
 *
 * @note The number of useable palettes depends on the palette format: 64 for
 * 4bit and 4 for 8bit
 */
int enj_font_PAL_PT_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                           uint8_t palette_entry, enj_color_t front_color,
                           enj_color_t back_color, pvr_palfmt_t pal_fmt);

/** Get UV coordinates for a glyph in a font
 * @param font Pointer to font header
 * @param glyph Character to get UVs for
 * @param auv Pointer to store top-left UV coordinate packed as uint32_t
 * @param buv Pointer to store top-right UV coordinate packed as uint32_t
 * @param cuv Pointer to store bottom-right UV coordinate packed as uint32_t
 *
 * @return 1 on success, 0 on failure
 *
 */
int enj_font_glyph_uv_coords(enj_font_header_t* font, char glyph, uint32_t* auv,
                             uint32_t* buv, uint32_t* cuv);

/** Render a single sprite glyph to the PVR command buffer
 * @param glyph Character to draw
 * @param font Pointer to font header
 * @param x X position to draw at in pixels
 * @param y Y position to draw at in pixels
 * @param state_ptr Optional pointer to PVR draw state
 *
 * @return width of rendered glyph in pixels
 *
 * @note Method assumes that the PVR sprite header has already been committed
 */
int enj_font_render_glyph(char glyph, enj_font_header_t* font, uint16_t x,
                          uint16_t y, pvr_dr_state_t* state_ptr);

/**
 * Render a text string to the PVR command buffer
 * @param text Null-terminated string to draw
 * @param font Pointer to font header
 * @param x X position to draw at in pixels
 * @param y Y position to draw at in pixels
 * @param state_ptr Optional pointer to PVR draw state

 * @return width of rendered text in pixels
 */
int enj_font_render_text(const char* text, enj_font_header_t* font, uint16_t x,
                         uint16_t y, pvr_dr_state_t* state_ptr);

/** Render text within a bounding box
 * @param text Null-terminated string to draw
 * @param font Pointer to font header
 * @param min_x X position of top-left corner of box in pixels
 * @param min_y Y position of top-left corner of box in pixels
 * @param box_width Width of bounding box in pixels
 * @param box_height Height of bounding box in pixels
 * @param state_ptr Optional pointer to PVR draw state
 *
 * @return number of lines rendered
 */
int enj_font_render_text_in_box(const char* text, enj_font_header_t* font,
                                uint16_t min_x, uint16_t min_y,
                                uint16_t box_width, uint16_t box_height,
                                pvr_dr_state_t* state_ptr);


/** Calculate the width of a text string in pixels
 * @param text Null-terminated string to measure
 * @param font Pointer to font header
 * @return width of text in pixels
 */
int enj_font_text_width(const char* text, enj_font_header_t* font);

/**
 *  Convert an enDjinn font to a 16-bit PVR texture
 * @param font Pointer to font header
 * @param mode Pixel mode to convert to (ARGB1555, ARGB4444,
 * RGB565)
 * @param front_color Color to use for the front of the glyphs
 * @param back_color Color to use for the background of the glyphs
 * @return Pointer to allocated PVR texture data, or NULL on failure
 */
pvr_ptr_t enj_font_to_16bit_texture(enj_font_header_t* font,
                                    pvr_pixel_mode_t mode,
                                    enj_color_t front_color,
                                    enj_color_t back_color);


#endif  // ENJ_FONTS_H
