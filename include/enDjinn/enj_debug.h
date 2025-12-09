#ifndef ENJ_DEBUG_H
#define ENJ_DEBUG_H

int enj_debug_init();
void enj_debug_shutdown();

/**
 * Writes a string to the screen using a built-in debug font.
 * @param str The string to write
 * @param x The x position in pixels
 * @param y The y position in pixels
 * @param zvalue The z value to use for rendering
 * @return The width of the rendered string in pixels
 */
int enj_debug_write_on_screen(const char* str, int x, int y, float zvalue);

#endif // ENJ_DEBUG_H