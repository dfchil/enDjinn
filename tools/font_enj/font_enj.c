#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb/stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */

typedef struct {
  uint16_t line : 4;
  uint16_t offset : 11;
  uint16_t available : 1;
} enj_glyph_offset_t;

typedef struct {
  struct {
    uint16_t log2width : 4;
    uint16_t log2height : 4;
    uint16_t line_height : 7;
    uint16_t flags : 5;
  };
  enj_glyph_offset_t glyph_starts[-33 + 127 + 1];
} enj_font_header_t;

static int intsort(const void *p1, const void *p2) {
  uint32_t v1 = *(uint32_t *)p1;
  uint32_t v2 = *(uint32_t *)p2;
  if (v1 < v2)
    return -1;
  else if (v1 > v2)
    return 1;
  else
    return 0;
}

int main(int argc, const char *argv[]) {
  int l_h = 17; /* line height */

  printf("sizeof enj_font_header_t: %zu\n", sizeof(enj_font_header_t));
  /* load font file */
  long size;
  uint8_t *fontBuffer;
  FILE *fontFile = fopen("font/DejaVuSans.ttf", "rb");
  // FILE* fontFile = fopen("font/cmunrm.ttf", "rb");
  // FILE* fontFile = fopen("font/DinaRemaster-Regular-01.ttf", "rb");

  fseek(fontFile, 0, SEEK_END);
  size = ftell(fontFile);       /* how long is the file ? */
  fseek(fontFile, 0, SEEK_SET); /* reset */

  fontBuffer = malloc(size);

  fread(fontBuffer, size, 1, fontFile);
  fclose(fontFile);

  /* prepare font */
  stbtt_fontinfo info;
  if (!stbtt_InitFont(&info, fontBuffer, 0)) {
    printf("failed\n");
  }

  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

  /* calculate font scaling */
  float scale = stbtt_ScaleForPixelHeight(&info, l_h);

  ascent = roundf(ascent * scale);
  descent = roundf(descent * scale);

  int x = 0;
  int line = 0;

  enj_font_header_t header = {0};

  int width = 64;
  int height = 64;
  int found_dimensions = 0;
  while (!found_dimensions) {
    printf("height: %d, width: %d\n", height, width);
    x = 0;
    line = 0;
    // calculate bitmap size
    char glyph = 33;
    for (; glyph < 127; ++glyph) {
      /* how wide is this character */
      int ax;
      int lsb;
      stbtt_GetCodepointHMetrics(&info, glyph, &ax, &lsb);
      int c_x1, c_y1, c_x2, c_y2;
      stbtt_GetCodepointBitmapBox(&info, glyph, scale, scale, &c_x1, &c_y1,
                                  &c_x2, &c_y2);
      if (x + (ax * scale) > width) {
        x = 0;
        ++line;
      }
      int y = ascent + c_y1;

      int byteOffset =
          x + roundf(lsb * scale) + (y * width * 2) + (line * l_h * width);

      if (byteOffset > width * height) {
        if (width < height)
          width <<= 1;
        else
          height <<= 1;
        break;
      }

      /* advance x */
      x += roundf(ax * scale) + 2;
      // int kern = stbtt_GetCodepointKernAdvance(&info, glyph, glyph +
      // 1); printf ("kern %d\n", kern); x += roundf(kern * scale);
    }
    if (glyph == 127) {
      found_dimensions = 1;
      printf("final height: %d, width: %d\n", height, width);

      break;
    }
  }
  if (found_dimensions == 0) {
    printf("bitmap cannot contain all glyphs\n");
    return 1;
  }

  /* create a bitmap for the phrase */
  uint8_t *bitmap = calloc(1, height * width * sizeof(uint8_t));

  x = 0;
  line = 0;
  for (char glyph = 33; glyph < 127; ++glyph) {
    /* how wide is this character */
    int ax;
    int lsb;
    stbtt_GetCodepointHMetrics(&info, glyph, &ax, &lsb);
    /* (Note that each Codepoint call has an alternative Glyph version which
     * caches the work required to lookup the character word[i].) */

    /* get bounding box for character (may be offset to account for chars
     * that dip above or below the line) */
    int c_x1, c_y1, c_x2, c_y2;
    stbtt_GetCodepointBitmapBox(&info, glyph, scale, scale, &c_x1, &c_y1, &c_x2,
                                &c_y2);
    if (x + (ax * scale) > width) {
      x = 0;
      ++line;
    }

    /* compute y (different characters have different heights) */
    int y = ascent + c_y1;

    /* render character (stride and offset is important here) */
    int byteOffset =
        x + roundf(lsb * scale) + (y * width) + (line * l_h * width);
    stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1,
                              c_y2 - c_y1, width, scale, scale, glyph);

    // printf("x:%d y:%d (%d..%d x %d..%d) ax:%d lsb:%d\n", x,
    //        y + (line * l_h), c_x1, c_x2, c_y1, c_y2, ax, lsb);

    /* advance x */
    x += roundf(ax * scale) + 2;

    /* add kerning */
    // x += roundf(kern * scale);
    // int kern = stbtt_GetCodepointKernAdvance(&info, glyph, glyph + 1);
    header.glyph_starts[glyph - 33] = (enj_glyph_offset_t){
        .available = 1, .line = (uint16_t)line, .offset = (uint16_t)(x)};
  }

  // make it 16 colors with alpha
  uint8_t *output = calloc(1, height * width * sizeof(uint8_t));
  uint8_t *pvrout = calloc(1, (height * width * sizeof(uint8_t)) >> 1);

  uint32_t new_palette[16] = {0};
  uint32_t new_count[16] = {0};
  uint32_t other_count[16] = {0};
  uint8_t colors_found = 1;
  for (int i = 0; i < height * width; i += 1) {
    uint8_t v1 = bitmap[i];

    uint8_t curcol = ((v1 * 15 + 135) >> 4) & 0xF0;
    uint8_t other_index =
        (v1 >> 4) + ((v1 & 0xf0) < 240 && ((v1 & 0x0f) > 8) ? 1 : 0);
    other_count[other_index]++;

    output[i] = curcol;
    // pvrout[i>>1] |= i & 1 ? (curcol >> 4) : (curcol & 0xF0);
    new_count[curcol >> 4]++;
  }
  // qsort(new_palette, 16, sizeof(uint32_t), intsort);
  for (int i = 0; i < 16; ++i) {
    printf("final palette %d: %d, count: %u, alt_count %u\n", i, new_palette[i],
           new_count[i], other_count[i]);
  }

  /* save out a 1 channel image */
  stbi_write_png("out.png", width, height, 1, output, width);

  /*
   Note that this example writes each character directly into the target image
   buffer. The "right thing" to do for fonts that have overlapping characters
   is MakeCodepointBitmap to a temporary buffer and then alpha blend that onto
   the target image. See the stb_truetype.h header for more info.
  */

  /* write out enj font file */

  header.log2width = 2;
  while ((1 << header.log2width) <= width) {
    printf("header.log2width: %d\n", 1 << header.log2width);
    header.log2width++;
  }
  header.log2height = 2;
  while ((1 << header.log2height) <= height) {
    printf("header.log2height: %d\n", 1 << header.log2height);
    header.log2height++;
  }
  header.line_height = l_h;
  FILE *outFile = fopen("out.enjfont", "wb");
  fwrite(&header, 1, sizeof(enj_font_header_t), outFile);
  fwrite(pvrout, 1, (height * width) / 2, outFile);
  fclose(outFile);

  free(fontBuffer);
  free(bitmap);
  free(output);
  free(pvrout);

  return 0;
}
