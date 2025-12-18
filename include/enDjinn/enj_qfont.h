#ifndef ENJ_DEBUG_H
#define ENJ_DEBUG_H

#include <dc/pvr.h>

int enj_qfont_init();
void enj_debug_shutdown();

/**
 * Writes a string to the screen using a injected font.
 * @param str The string to write
 * @param x The x position in pixels
 * @param y The y position in pixels
 * @param cur_mode The PVR list that is currently being submitted to
 * @return The width of the rendered string in pixels
 *
 * @note The intended time to use this function is when while submitting to the
 * same list as the cur_mode argument. PVR_LIST_PT_POLY is recommended.
 * 
 * @note The built in font is 1 bit without gradients and is very suitable for integer scaling by calling
 * @see  @link enj_font_set_scale @endlink.
 */
int enj_qfont_write(const char* str, int x, int y, pvr_list_type_t cur_mode);

/**
 * Get the pointer to the injected font's PVR texture data
 * @return Pointer to PVR texture data
 */
pvr_ptr_t enj_qfont_get_pvr_ptr();

/**
 * Get the pointer to the injected font's header
 * @return Pointer to font header
 */
enj_font_header_t* enj_qfont_get_header();

/**
 * Get the pointer to the injected font's sprite header
 * @return Pointer to sprite header
 * 
 * @note The sprite header will be configured for PVR_LIST_PT_POLY by default
 * but will be reconfigured if used with enj_qfont_write with a different list
 * type.
 */
pvr_sprite_hdr_t* enj_qfont_get_sprite_hdr();

/**
 * Set the color of the sprite that glyphs are rendered with
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * 
 * @note The effect depends on the texture mode and pvr_list_type being used.
 */
void enj_qfont_set_color(uint8_t r, uint8_t g, uint8_t b);

#endif  // ENJ_DEBUG_H