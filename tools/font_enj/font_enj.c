#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb/stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */


#include "../../include/enDjinn/enj_font_types.h"

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

int calculate_sheet_sizes(int line_height,stbtt_fontinfo *info,
                         int *out_width, int *out_height) {
  int sheet_width = 64;
  int sheet_height = 64;
  int found_sizes = 0;
  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);
  /* calculate font scaling */
  float scale = stbtt_ScaleForPixelHeight(info, line_height);
  ascent = roundf(ascent * scale);
  descent = roundf(descent * scale);

  while (!found_sizes) {
    int x = 0;
    int line = 0;
    // calculate bitmap size
    char glyph = 33;
    for (; glyph < 127; ++glyph) {
      /* how wide is this character */
      int ax;
      int lsb;
      stbtt_GetCodepointHMetrics(info, glyph, &ax, &lsb);
      int c_x1, c_y1, c_x2, c_y2;
      stbtt_GetCodepointBitmapBox(info, glyph, scale, scale, &c_x1, &c_y1,
                                  &c_x2, &c_y2);
      if (x + (ax * scale) > sheet_width) {
        x = 0;
        ++line;
      }
      int y = ascent + c_y1;

      int byteOffset = x + roundf(lsb * scale) + (y * sheet_width * 2) +
                       (line * line_height * sheet_width);

      if (byteOffset > sheet_width * sheet_height) {
        if (sheet_height < sheet_width)
          sheet_height <<= 1;
        else
          sheet_width <<= 1;
        break;
      }

      /* advance x */
      x += roundf(ax * scale);
      int kern = stbtt_GetCodepointKernAdvance(info, glyph, glyph + 1);
      x += roundf(kern * scale);
    }
    if (glyph == 127) {
      found_sizes = 1;
      printf("final sheet_height: %d, sheet_width: %d\n", sheet_height, sheet_width);

      break;
    }
  }
  if (found_sizes == 0) {
    printf("bitmap cannot contain all glyphs\n");
    return 0;
  }
  *out_width = sheet_width;
  *out_height = sheet_height;
  return 1;
}

int read_font_file(const char *path, uint8_t **out_buffer) {
  FILE *fontFile = fopen(path, "rb");
  if (!fontFile) {
    printf("failed to open font file: %s\n", path);
    return 0;
  }

  fseek(fontFile, 0, SEEK_END);
  long size = ftell(fontFile);       /* how long is the file ? */
  fseek(fontFile, 0, SEEK_SET); /* reset */

  uint8_t *fontBuffer = malloc(size);
  if (!fontBuffer) {
    printf("failed to allocate font buffer\n");
    fclose(fontFile);
    return 0;
  }

  fread(fontBuffer, size, 1, fontFile);
  fclose(fontFile);

  *out_buffer = fontBuffer;
  return 1;
}

int font_gen(int line_height, const char *font_path, const char *output_path) {
  enj_font_header_t header = {0};

  /* prepare font */
  uint8_t *fontBuffer = NULL;
  read_font_file(font_path, &fontBuffer);
  stbtt_fontinfo info;
  if (!stbtt_InitFont(&info, fontBuffer, 0)) {
    printf("failed\n");
  }
  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
  /* calculate font scaling */
  float scale = stbtt_ScaleForPixelHeight(&info, line_height);
  ascent = roundf(ascent * scale);
  descent = roundf(descent * scale);

  int sheet_width;
  int sheet_height;
  if (!calculate_sheet_sizes(line_height, &info, &sheet_width, &sheet_height)) {
    return -1;
  }
  uint8_t *bitmap = calloc(1, sheet_height * sheet_width * sizeof(uint8_t));
  int cur_pos_x = 0;
  int cur_line = 0;
  for (char glyph = 33; glyph < 127; ++glyph) {
    /* how wide is this character */
    int advance_x;
    int lsb;
    stbtt_GetCodepointHMetrics(&info, glyph, &advance_x, &lsb);
    /* (Note that each Codepoint call has an alternative Glyph version which
     * caches the work required to lookup the character word[i].) */

    /* get bounding box for character (may be offset to account for chars
     * that dip above or below the line) */
    int c_x1, c_y1, c_x2, c_y2;
    stbtt_GetCodepointBitmapBox(&info, glyph, scale, scale, &c_x1, &c_y1, &c_x2,
                                &c_y2);
    if (cur_pos_x + (advance_x * scale) > sheet_width) {
      cur_pos_x = 0;
      ++cur_line;
    }
    /* compute y (different characters have different heights) */
    int y = ascent + c_y1;

    /* render character (stride and offset is important here) */
    int byteOffset =
        cur_pos_x + roundf(lsb * scale) + (y * sheet_width) + (cur_line * line_height * sheet_width);
    stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1,
                              c_y2 - c_y1, sheet_width, scale, scale, glyph);

    /* advance x */
    cur_pos_x += roundf(advance_x * scale);

    /* add kerning */
    int kern = stbtt_GetCodepointKernAdvance(&info, glyph, glyph + 1);
    cur_pos_x += roundf(kern * scale);
    header.glyph_starts[glyph - 33] = (enj_glyph_offset_t){
        .available = 1, .line = (uint16_t)cur_line, .offset = (uint16_t)(cur_pos_x)};
  }

  // 4 bits per pixel to PVR4bpp
  uint8_t *pvrout = calloc(1, (sheet_height * sheet_width * sizeof(uint8_t)) >> 1);

  for (int i = 0; i < sheet_height * sheet_width; i += 1) {
    uint8_t v1 = bitmap[i];
    uint8_t curcol = ((v1 * 15 + 135) >> 4) & 0xF0;
    bitmap[i] = curcol;
    pvrout[i >> 1] |= i & 1 ? (curcol >> 4) : (curcol & 0xF0);
  }

  /* save out a 1 channel image */
  stbi_write_png("out.png", sheet_width, sheet_height, 1, bitmap, sheet_width);

  /* write out enj font file */
  header.log2height = 5; // start at 32
  while ((1 << header.log2height) < sheet_height) {
    header.log2height++;
  };
  header.log2width = header.log2height; // width is always >= height
  while ((1 << header.log2width) < sheet_width) {
    header.log2width++;
  };
  header.line_height = line_height;

  FILE *outFile = fopen(output_path, "wb");
  fwrite(&header, 1, sizeof(enj_font_header_t), outFile);
  fwrite(pvrout, 1, (sheet_height * sheet_width) / 2, outFile);
  fclose(outFile);

  free(fontBuffer);
  free(bitmap);
  free(pvrout);

  return 0;
}

int main(int argc, const char *argv[]) { return font_gen(23, "font/DejaVuSans.ttf", "out.enjfont"); }