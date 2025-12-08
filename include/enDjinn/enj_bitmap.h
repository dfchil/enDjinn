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

/**
 * Create a new bitmap with given width and height
 * Width and height must be multiples of 8
 * @param width Width of the bitmap in pixels
 * @param height Height of the bitmap in pixels
 * @return Pointer to the created bitmap, or NULL on failure
 */
enj_bitmap_t *enj_bitmap_create(int width, int height);

/**
 * Destroy a bitmap and free its memory
 * @param bitmap Pointer to the bitmap to destroy
 */
void enj_bitmap_destroy(enj_bitmap_t *bitmap);

/**
 * Set a pixel in the bitmap
 * @param bmap Pointer to the bitmap
 * @param x X coordinate of the pixel to set
 * @param y Y coordinate of the pixel to set
 */
void enj_bitmap_set(enj_bitmap_t *bmap, int x, int y);

/**
 * Clear a pixel in the bitmap
 * @param bmap Pointer to the bitmap
 * @param x X coordinate of the pixel to clear
 * @param y Y coordinate of the pixel to clear
 */
void enj_bitmap_clear(enj_bitmap_t *bmap, int x, int y);

/**
 * Write a line to the bitmap
 * @param bmap Pointer to the bitmap
 * @param line Line parameters
 *
 * @note Lines are limited to either axis-aligned or 45-degree diagonals
 */
void enj_bitmap_write_line(enj_bitmap_t *bmap, enj_bitmap_line_t line);

/**
 * Reset the bitmap to all pixels cleared
 * @param bmap Pointer to the bitmap
 */
void enj_bitmap_reset(enj_bitmap_t *bmap);

/**
 * Save the bitmap to a PNM file
 * @param filename Name of the file to save to
 * @param bitmap Pointer to the bitmap
 */
void enj_bitmap_to_pnm(const char *filename, enj_bitmap_t *bitmap);

#endif // ENJ_BITMAP_H