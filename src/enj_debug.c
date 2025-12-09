#include <enDjinn/enj_debug.h>
#include <enDjinn/enj_font.h>
#include <enDjinn/render.h>

// #define ENJ_ADD_DBG_FONT
#ifdef ENJ_ADD_DBG_FONT
static enj_font_header_t enj_debug_font;
static pvr_sprite_hdr_t enj_debug_font_sprite_hdr;

// #ifdef ENJ_DIR
// uint8_t enj_debug_font_data[] = {
// #embed ENJ_DIR "embeds/debug_font.enjfont"
// };
// #else
// #endif
#include <enDjinn/debug_font.h>
#include "enj_debug.h"

int enj_debug_init() {
  enj_font_from_blob(enj_debug_font_data, &debug_font);
  enj_font_PAL_OP_header(&debug_font, &enj_debug_font_sprite_hdr, 0,
                      (enj_color_t){.raw = 0xffffffff},
                      (enj_color_t){.raw = 0x00000000}, PVR_PAL_ARGB8888);
  return 0;
}

int enj_debug_write_on_screen(const char *str, int x, int y, float zvalue)
{
    enj_renderlist_add(PVR_LIST_PT_POLY, NULL, NULL);
    return enj_font_render_text(str, &debug_font, x, y, 10.0f, NULL);
}
else int enj_debug_init()
{
    return 0;
}
#endif // ENJ_ADD_DBG_FONT
