#ifndef ENJ_DEBUG_H
#define ENJ_DEBUG_H

int enj_debug_init();
void enj_debug_shutdown();

int enj_debug_write_on_screen(const char* str, int x, int y);

#endif // ENJ_DEBUG_H