#include <enDjinn/enj_debug.h>
#include <enDjinn/enj_draw.h>
#include <enDjinn/enj_font.h>
#include <enDjinn/enj_render.h>

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

int enj_debug_init() {
  enj_font_from_blob(enj_debug_font_data, &enj_debug_font);
  enj_font_PAL_OP_header(&enj_debug_font, &enj_debug_font_sprite_hdr, 0,
                         (enj_color_t){.raw = 0xffffffff},
                         (enj_color_t){.raw = 0x00000000}, PVR_PAL_ARGB8888);
  return 0;
}

int enj_debug_write_on_screen(const char *str, int x, int y) {
  enj_renderlist_add(PVR_LIST_PT_POLY, NULL, NULL);
  return enj_font_render_text(str, &enj_debug_font, x, y, NULL);
}
void enj_debug_shutdown() {
  // Nothing to do
}

#endif // ENJ_ADD_DBG_FONT
