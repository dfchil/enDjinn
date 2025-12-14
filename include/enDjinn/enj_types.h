#ifndef ENJ_TYPES_H
#define ENJ_TYPES_H

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