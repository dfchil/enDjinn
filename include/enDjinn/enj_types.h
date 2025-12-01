#ifndef ENJ_TYPES_H
#define ENJ_TYPES_H

#include <stdint.h>

typedef union {
  uint32_t raw; // 0xAABBGGRR
  struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
  };
} enj_color_t;

#endif // ENJ_TYPES_H