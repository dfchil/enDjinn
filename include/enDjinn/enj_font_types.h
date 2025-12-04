#ifndef ENJ_FONT_TYPES_H
#define ENJ_FONT_TYPES_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
  uint16_t line : 4;
  uint16_t offset_end : 11;
  uint16_t available : 1;
} enj_glyph_offset_t;

typedef struct __attribute__((packed)) {
  struct {
    uint16_t log2width : 4;
    uint16_t log2height : 4;
    uint16_t line_height : 8;
  };
  uint32_t pvr_data;
  enj_glyph_offset_t glyph_starts['~' - '!'];
} enj_font_header_t;

#endif // ENJ_FONT_TYPES_H