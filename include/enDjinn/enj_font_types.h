#ifndef ENJ_FONT_TYPES_H
#define ENJ_FONT_TYPES_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
  uint16_t line : 4;
  uint16_t offset : 11;
  uint16_t available : 1;
} enj_glyph_offset_t;

typedef struct __attribute__((packed)) {
  struct {
    uint8_t log2width : 4;
    uint8_t log2height : 4;
  };
  uint8_t line_height;
  /* from '!' to '~', last +1 is padding */
  uint32_t pvr_data;
  enj_glyph_offset_t glyph_starts[-33 + 126];
} enj_font_header_t;

#endif // ENJ_FONT_TYPES_H