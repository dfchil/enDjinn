#ifndef ENJ_QFONT_H
#define ENJ_QFONT_H

#ifdef ENJ_INJECT_QFONT

#include <kos.h>
#include <enDjinn/enj_api.h>
#include <enDjinn/enj_font_types.h>

ENJ_BEGIN_DECLS

/** Initialize the injected quick font. @return 0 on success, -1 on failure. */
int enj_qfont_init(void);

/** Release the injected quick font's PVR texture. Safe to call repeatedly. */
void enj_qfont_shutdown(void);

/**
 * Write a string using the injected font.
 * @param str The string to write
 * @param x The x position in pixels
 * @param y The y position in pixels
 * @param cur_mode The PVR list that is currently being submitted to
 * @return The width of the rendered string in pixels
 *
 * @note Call this while submitting the same list as cur_mode.
 * PVR_LIST_PT_POLY is recommended.
 * 
 * @note The built-in font is one-bit and well suited to integer scaling with
 * enj_font_scale_set().
 */
int enj_qfont_write(const char* str, int x, int y, pvr_list_type_t cur_mode);

/**
 * Get the pointer to the injected font's PVR texture data
 * @return Pointer to PVR texture data
 */
pvr_ptr_t enj_qfont_get_pvr_ptr(void);

/**
 * Get the pointer to the injected font's header
 * @return Pointer to font header
 */
enj_font_header_t* enj_qfont_get_header(void);

/**
 * Get the pointer to the injected font's sprite header
 * @return Pointer to sprite header
 * 
 * @note The sprite header will be configured for PVR_LIST_PT_POLY by default
 * but will be reconfigured if used with enj_qfont_write with a different list
 * type.
 */
pvr_sprite_hdr_t* enj_qfont_get_sprite_hdr(void);

/**
 * Set the color of the sprite that glyphs are rendered with
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * 
 * @note The effect depends on the texture mode and pvr_list_type being used.
 */
void enj_qfont_color_set(uint8_t r, uint8_t g, uint8_t b);

ENJ_END_DECLS
#endif  // ENJ_INJECT_QFONT

#endif  // ENJ_QFONT_H
