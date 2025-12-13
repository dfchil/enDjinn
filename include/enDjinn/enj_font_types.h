#ifndef ENJ_FONT_TYPES_H
#define ENJ_FONT_TYPES_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
  uint16_t line : 5;
  uint16_t x_min : 10; // max 1024 width
  uint16_t available : 1;
} enj_glyph_offset_t;

#define ENJ_FONT_4BIT_PALETTE 0
#define ENJ_FONT_1BIT_PALETTE 1

typedef struct __attribute__((packed)) {
  struct {
    uint16_t major : 2;
    uint16_t minor : 6;
    uint16_t patch : 8;
  } version;
  struct {
    uint32_t log2width : 4;
    uint32_t log2height : 4;
    uint32_t line_height : 10;
    uint32_t palette_type: 1;
    uint32_t reserved : 13;
  };
  uint32_t pvr_data;
  enj_glyph_offset_t glyph_endings['~' - '!' + 2]; // 1 extra for <= and one for ending
  enj_glyph_offset_t special_glyphs[12];
} enj_font_header_t;

#endif // ENJ_FONT_TYPES_H