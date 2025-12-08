#ifndef _ENJ_TYPES_H_
#define _ENJ_TYPES_H_

typedef union {
  uint32 raw; // 0xBBGGRRAA
  struct {
    uint8 b;
    uint8 g;
    uint8 r;
    uint8 a;
  };
} enj_color_t;

#endif /* _ENJ_TYPES_H_ */