#include <enDjinn/enj_font.h>

int enj_font_load_from_file(const char* path, enj_font_header_t* out_font) {
    return 0;
}

int enj_font_TR_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                       uint8_t palette_offset, enj_color_t front_color) {
    return 0;
}

int enj_font_OP_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                       uint8_t palette_offset, enj_color_t front_color,
                       enj_color_t back_color) {
    return 0;
}

int enj_font_PT_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                       uint8_t palette_offset, enj_color_t front_color,
                       enj_color_t back_color) {
    return 0;
}

int enj_font_glyph_uv_coords(enj_font_header_t* font, char glyph, float* u0,
    float* v0, float* u1, float* v1) {
    return 0;
}

int enj_font_render_glyph(char glyph, enj_font_header_t* font,
                          pvr_sprite_hdr_t* hdr, uint16_t x, uint16_t y) {
    return 0;
}

int enj_font_render_text(const char* text, enj_font_header_t* font,
                         pvr_sprite_hdr_t* hdr, uint16_t x, uint16_t y) {
    return 0;
}

int enj_font_render_text_in_box(const char* text, enj_font_header_t* font,
                             pvr_sprite_hdr_t* hdr, uint16_t x, uint16_t y) {}

int enj_font_text_width(const char* text, enj_font_header_t* font) { return 0; }
