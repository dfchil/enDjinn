#ifndef ENJ_BITMAP_H
#define ENJ_BITMAP_H
#include <stdint.h>

typedef struct {
  uint32_t start_x : 9;
  int32_t start_y : 8;
  uint32_t length : 9;
  int32_t direction_x : 3;
  int32_t direction_y : 3;
} enj_bitmap_line_t;

typedef struct {
  int width;
  int height;
  int8_t *data;
} enj_bitmap_t;

enj_bitmap_t *enj_bitmap_create(int width, int height);
void enj_bitmap_destroy(enj_bitmap_t *bitmap);
void enj_bitmap_set(enj_bitmap_t *bmap, int x, int y);
void enj_bitmap_clear(enj_bitmap_t *bmap, int x, int y);
void enj_bitmap_write_line(enj_bitmap_t *bmap, enj_bitmap_line_t line);
void enj_bitmap_reset(enj_bitmap_t *bmap);
void enj_bitmap_to_pnm(const char *filename, enj_bitmap_t *bitmap);

#endif // ENJ_BITMAP_H