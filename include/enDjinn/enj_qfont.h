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
 */
int enj_qfont_write(const char *str, int x, int y, pvr_list_type_t cur_mode);

#endif // ENJ_DEBUG_H