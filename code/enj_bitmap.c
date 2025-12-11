
#include <enDjinn/enj_enDjinn.h>

#include <malloc.h>
#include <string.h>

#define BITS_PER_BYTE 8
#define BYTE_OFFSET(b) ((b) / BITS_PER_BYTE)
#define BIT_OFFSET(b) ((b) % BITS_PER_BYTE)

enj_bitmap_t *enj_bitmap_create(int width, int height) {
  if (width % 8 != 0) {
    ENJ_DEBUG_PRINT("Width must be a multiple of 8");
    return NULL;
  }
  if (height % 8 != 0) {
    ENJ_DEBUG_PRINT("Height must be a multiple of 8");
    return NULL;
  }

  enj_bitmap_t *bitmap =
      calloc(sizeof(enj_bitmap_t) + ((width * height) / BITS_PER_BYTE), 1);
  if (bitmap == NULL) {
    return NULL;
  }
  bitmap->width = width;
  bitmap->height = height;
  bitmap->data = (int8_t *)((size_t)bitmap + sizeof(enj_bitmap_t));
  return bitmap;
}

void enj_bitmap_reset(enj_bitmap_t *bmap) {
  if (bmap == NULL) {
    return;
  }
  memset(bmap->data, 0, (bmap->width * bmap->height) / BITS_PER_BYTE);
}

void enj_bitmap_destroy(enj_bitmap_t *bitmap) {
  if (bitmap != NULL) {
    free(bitmap);
  }
}

void enj_bitmap_set(enj_bitmap_t *bmap, int x, int y) {
  if (x < 0 || x >= bmap->width || y < 0 || y >= bmap->height) {
    return;
  }
  int bit_offset = ((y * bmap->width) + x);
  if (bit_offset >= (bmap->width * bmap->height)) {
    ENJ_DEBUG_PRINT("enj_bitmap_set: x: %d y:%d offset:%d\n", x, y, bit_offset);
    return;
  }
  int byte_index = BYTE_OFFSET(bit_offset);
  int bit_index = BIT_OFFSET(bit_offset);
  bmap->data[byte_index] |= (1 << (7 - bit_index));
}

void enj_bitmap_clear(enj_bitmap_t *bmap, int x, int y) {
  if (x < 0 || x >= bmap->width || y < 0 || y >= bmap->height) {
    return;
  }
  int bit_offset = (x * bmap->width + y) * BITS_PER_BYTE;
  int byte_index = BYTE_OFFSET(bit_offset);
  int bit_index = BIT_OFFSET(bit_offset);
  bmap->data[byte_index] &= ~(1 << bit_index);
}

void enj_bitmap_write_line(enj_bitmap_t *bmap, enj_bitmap_line_t line) {
  for (int j = 0; j <= line.length; j++) {
    enj_bitmap_set(bmap, line.start_x + j * line.direction_x,
                   line.start_y + j * line.direction_y);
  }
}

void enj_bitmap_to_pnm(const char *filename, enj_bitmap_t *bitmap) {

  FILE *file = fopen(filename, "wb");
  if (file == NULL) {
    perror("Failed to open file");
    return;
  }

  fprintf(file, "P4\n%d %d\n", bitmap->width, bitmap->height);
  fwrite(bitmap->data, 1, (bitmap->width * bitmap->height) / BITS_PER_BYTE,
         file);

  fclose(file);
}
