#ifndef ENJ_TYPES_H
#define ENJ_TYPES_H

#include <stdint.h>

/**
 * 32-bit PVR color with named ARGB channels.
 *
 * On enDjinn's little-endian targets, raw uses the conventional 0xAARRGGBB
 * representation while the byte fields provide direct channel access.
 */
typedef union {
  uint32_t raw; // 0xAARRGGBB
  struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
  };
} enj_color_t;

#endif /* ENJ_TYPES_H */
