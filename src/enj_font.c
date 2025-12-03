#include <enDjinn/enj_draw.h>
#include <enDjinn/enj_font.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int enj_font_from_blob(const uint8_t* blob, enj_font_header_t* out_font) {
    memcpy(out_font, blob, sizeof(enj_font_header_t));
    uint8_t* pvr_data = pvr_mem_malloc(
        ((1 << (out_font->log2width) * (1 << out_font->log2height))) >> 1);
    if (!pvr_data) {
        printf("Error allocating memory for font PVR data\n");
        return 0;
    }
    pvr_txr_load_ex(blob + sizeof(enj_font_header_t), pvr_data,
                    1 << out_font->log2width, 1 << out_font->log2height,
                    PVR_TXRLOAD_4BPP);
    out_font->pvr_data = (uint32_t)(uintptr_t)pvr_data;

    return 1;
}

int enj_font_load(const char* path, enj_font_header_t* out_font) {
    int success = 1;
    FILE* file = NULL;
    do {
        file = fopen(path, "rb");
        if (!file) {
            printf("Error opening font file %s: %s\n", path, strerror(errno));
            success = 0;
            break;
        }

        if (fread(out_font, sizeof(enj_font_header_t), 1, file) != 1) {
            printf("Error reading font header from file %s\n", path);
            success = 0;
            break;
        }
        size_t blobsize =
            sizeof(enj_font_header_t) +
            (((1 << (out_font->log2width) * (1 << out_font->log2height))) >> 1);
        uint8_t* font_blob = memalign(32, blobsize);
        if (!font_blob) {
            printf("Error allocating memory for font blob from file %s\n",
                   path);
            success = 0;
            break;
        }
        memcpy(font_blob, out_font, sizeof(enj_font_header_t));
        if (fread(font_blob + sizeof(enj_font_header_t),
                  blobsize - sizeof(enj_font_header_t), 1, file) != 1) {
            printf("Error reading font blob from file %s\n", path);
            free(font_blob);
            success = 0;
            break;
        }
        success = enj_font_from_blob(font_blob, out_font);
        free(font_blob);
    } while (0);

    if (file != NULL) {
        fclose(file);
    }
    return success;
}

int enj_font_TR_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                       uint8_t palette_entry, enj_color_t front_color, pvr_palfmt_t pal_fmt) {
    // generate transparent palette
    uint32_t palette_offset = palette_entry << (pal_fmt == PVR_PAL_ARGB8888 ? 8 : 4);

    enj_color_t color = {.a = 0, .r = 255, .g = 255, .b = 255};
    for (int i = 0; i < 16; i++) {
        color.a = i * 17;  // 0, 17, 34, ..., 255
        pvr_set_pal_entry(palette_offset + i, color.raw);
    }

    // setup header
    pvr_sprite_cxt_t cxt;
    pvr_sprite_cxt_txr(
        &cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_PAL4BPP | (palette_entry << (pal_fmt == PVR_PAL_ARGB8888 ? 25 : 21)),
        1 << font->log2width, 1 << font->log2height,
        (pvr_ptr_t)(uintptr_t)font->pvr_data, PVR_FILTER_NEAREST);
    pvr_sprite_compile(hdr, &cxt);
    hdr->argb = front_color.raw;

    return 1;
}

static inline void palette_color_mixer(enj_color_t front_color,
                                       enj_color_t back_color,
                                       uint8_t palette_entry,
                                       pvr_palfmt_t pal_fmt
                                    ) {
    enj_color_t diff_color = {.a = 0,
                              .r = (back_color.r - front_color.r) / 15,
                              .g = (back_color.g - front_color.g) / 15,
                              .b = (back_color.b - front_color.b) / 15};
    uint32_t palette_offset = palette_entry << (pal_fmt == PVR_PAL_ARGB8888 ? 8 : 4);
    for (int i = 0; i < 16; i++) {
        enj_color_t color = {.a = 255,
                             .r = front_color.r + diff_color.r * i,
                             .g = front_color.g + diff_color.g * i,
                             .b = front_color.b + diff_color.b * i};
        pvr_set_pal_entry(palette_offset + i, color.raw);
    }
}

int enj_font_OP_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                       uint8_t palette_entry, enj_color_t front_color,
                       enj_color_t back_color, pvr_palfmt_t pal_fmt) {
    // generate opaque palette
    palette_color_mixer(front_color, back_color, palette_entry, pal_fmt);

    // setup header
    pvr_sprite_cxt_t cxt;
    pvr_sprite_cxt_txr(
        &cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_PAL4BPP | (palette_entry << 21),
        1 << font->log2width, 1 << font->log2height,
        (pvr_ptr_t)(uintptr_t)font->pvr_data, PVR_FILTER_NEAREST);
    pvr_sprite_compile(hdr, &cxt);

    return 1;
}

int enj_font_PT_header(enj_font_header_t* font, pvr_sprite_hdr_t* hdr,
                       uint8_t palette_entry, enj_color_t front_color,
                       enj_color_t back_color, pvr_palfmt_t pal_fmt) {
    // generate punchthrough palette
    palette_color_mixer(front_color, back_color, palette_entry, pal_fmt);

    // setup header
    pvr_sprite_cxt_t cxt;
    pvr_sprite_cxt_txr(
        &cxt, PVR_LIST_PT_POLY, PVR_TXRFMT_PAL4BPP | (palette_entry << 21),
        1 << font->log2width, 1 << font->log2height,
        (pvr_ptr_t)(uintptr_t)font->pvr_data, PVR_FILTER_NEAREST);
    pvr_sprite_compile(hdr, &cxt);

    return 1;
}

int enj_font_glyph_uv_coords(enj_font_header_t* font, char glyph, uint32_t* auv,
                             uint32_t* buv, uint32_t* cuv) {
    if (glyph == ' ') {
        *auv = PVR_PACK_16BIT_UV(0.999f, 0.999f);
        *buv = PVR_PACK_16BIT_UV(1.0f, 0.999f);
        *cuv = PVR_PACK_16BIT_UV(1.0f, 1.0f);
        return 1;
    }

    int glyph_index = (uint32_t)glyph - 33;
    if (glyph_index >= (126 - 33 || glyph_index < 0)) {
        // out of range
        return 0;
    }
    enj_glyph_offset_t glyph_end = font->glyph_starts[glyph_index];

    uint32_t glyph_start =
        glyph_index > 0 ? font->glyph_starts[glyph_index - 1].offset_end : 0;
    // uint32_t glyph_width = glyph_end.offset_end - glyph_start;
    uint32_t tex_width = 1 << font->log2width;
    uint32_t tex_height = 1 << font->log2height;

    *auv = PVR_PACK_16BIT_UV(
        (float)(glyph_start) / (float)tex_width,
        (float)(font->line_height * glyph_end.line) / (float)tex_height);
    *buv = PVR_PACK_16BIT_UV(
        (float)(glyph_end.offset_end) / (float)tex_width,
        (float)(font->line_height * glyph_end.line) / (float)tex_height);
    *cuv = PVR_PACK_16BIT_UV(
        (float)(glyph_end.offset_end) / (float)tex_width,
        (float)(font->line_height * (glyph_end.line + 1)) / (float)tex_height);

    return 1;
}

int enj_font_render_glyph(char glyph, enj_font_header_t* font, uint16_t x,
                          uint16_t y, float zvalue, pvr_dr_state_t* state_ptr) {
    if (glyph == ' ') {
        return font->line_height >> 1;
    }
    uint32_t glyph_index = (uint32_t)glyph - 33;
    if (glyph_index >= (126 - 33) || glyph_index < 32) {
        // out of range
        return 0;
    }

    enj_glyph_offset_t glyph_end = font->glyph_starts[glyph_index];
    uint32_t glyph_start =
        glyph_index > 0 ? font->glyph_starts[glyph_index - 1].offset_end : 0;

    float corners[4][3] = {
        {(float)x, (float)(y + font->line_height), zvalue},
        {(float)x, (float)y, zvalue},
        {(float)(x + (glyph_end.offset_end - glyph_start)), (float)y, zvalue},
        {(float)(x + (glyph_end.offset_end - glyph_start)),
         (float)(y + font->line_height), zvalue},
    };
    uint32_t texcoords[3];
    enj_font_glyph_uv_coords(font, glyph, &texcoords[0], &texcoords[1],
                             &texcoords[2]);
    enj_draw_sprite(corners, state_ptr, NULL, texcoords);
    return 1;
}

int enj_font_render_text(const char* text, enj_font_header_t* font, uint16_t x,
                         uint16_t y) {
    return 0;
}

int enj_font_render_text_in_box(const char* text, enj_font_header_t* font,
                                uint16_t x, uint16_t y, uint16_t box_width,
                                uint16_t box_height) {
    return 0;
}

int enj_font_text_width(const char* text, enj_font_header_t* font) { return 0; }
