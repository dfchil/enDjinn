#ifndef ENJ_FONTS_H
#define ENJ_FONTS_H

#include <dc/pvr.h>
#include <enDjinn/enj_types.h>

typedef struct {
    uint16_t dimensions;
    uint16_t line_height;
    uint16_t glyph_starts[127-33];
    uint32_t palette[16];
} enj_font_t;

// note: 
// make sure font palettes are well ordered
// so that enj can remap to different purposes (PT, TR, OP etc)

int enj_font_rewrite_palette(enj_font_t* font, pvr_list_type_t listtype, enj_color_t front_color, 
                        enj_color_t back_color, uint8_t palette_offset);

int enj_font_hdr_config(enj_font_t* font, pvr_sprite_hdr_t* hdr, pvr_list_type_t listtype, uint8_t palette_offset);


#endif // ENJ_FONTS_H
