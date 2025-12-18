#ifndef ENJ_TYPES_H
#define ENJ_TYPES_H

/**
 * Color type used by pvr 32 bits RGBA wrapped in a union for easy access
 * uint8_t fields
 */
typedef union {
  uint32 raw; // 0xBBGGRRAA
  struct {
    uint8 b;
    uint8 g;
    uint8 r;
    uint8 a;
  };
} enj_color_t;

#endif /* ENJ_TYPES_H */