#ifndef PC_ENDJINN_PVRTEX_FILE_DCTEX_H
#define PC_ENDJINN_PVRTEX_FILE_DCTEX_H

#include <stdint.h>

typedef enum fdtPixelFormat {
  FDT_FMT_ARGB1555,
  FDT_FMT_RGB565,
  FDT_FMT_ARGB4444,
  FDT_FMT_YUV,
  FDT_FMT_NORMAL,
  FDT_FMT_PALETTE_4BPP,
  FDT_FMT_PALETTE_8BPP,
} fdtPixelFormat;

typedef struct {
  char fourcc[4];
  uint32_t chunk_size;
  uint8_t version;
  uint8_t header_size;
  uint8_t codebook_size;
  uint8_t colors_used;
  uint16_t width_pixels;
  uint16_t height_pixels;
  uint32_t pvr_type;
  uint32_t pad1;
  uint32_t pad2;
  uint32_t pad3;
} fDtHeader;

static inline unsigned fDtGetPixelFormat(const fDtHeader *texture) {
  return (texture->pvr_type >> 27) & 7u;
}

static inline int fDtIsPalettized(const fDtHeader *texture) {
  unsigned format = fDtGetPixelFormat(texture);
  return format == FDT_FMT_PALETTE_4BPP ||
         format == FDT_FMT_PALETTE_8BPP;
}

static inline int fDtIsCompressed(const fDtHeader *texture) {
  return (texture->pvr_type & (1u << 30)) != 0u;
}

static inline int fDtIsMipmapped(const fDtHeader *texture) {
  return (texture->pvr_type & (1u << 31)) != 0u;
}

static inline int fDtIsStrided(const fDtHeader *texture) {
  return !fDtIsPalettized(texture) &&
         (texture->pvr_type & (1u << 25)) != 0u;
}

static inline int fDtIsTwiddled(const fDtHeader *texture) {
  return fDtIsPalettized(texture) ||
         (texture->pvr_type & (1u << 26)) == 0u;
}

static inline unsigned fDtGetColorsUsed(const fDtHeader *texture) {
  return fDtIsPalettized(texture) ? (unsigned)texture->colors_used + 1u : 0u;
}

static inline unsigned fDtGetPvrWidth(const fDtHeader *texture) {
  return 1u << (((texture->pvr_type >> 3) & 7u) + 3u);
}

static inline unsigned fDtGetPvrHeight(const fDtHeader *texture) {
  return 1u << ((texture->pvr_type & 7u) + 3u);
}

#endif
