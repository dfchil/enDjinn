#include <enDjinn/enj_debug.h>
#include <enDjinn/enj_font.h>


// #define ENJ_ADD_DBG_FONT
#ifdef ENJ_ADD_DBG_FONT
static enj_font_header_t debug_font;

#ifndef ENJ_BUILDDIR
#error "ENJ_BUILDDIR not defined"
#endif

// uint8_t debug_font_data[] = {
// #embed  ENJ_BUILDDIR "/enj_debug/font/debug_font.dt"
// };


int enj_debug_init() {
    // enj_font_load_from 
    
    return 0;
}
#else
int enj_debug_init() {
    return 0;
}
#endif // ENJ_ADD_DBG_FONT


