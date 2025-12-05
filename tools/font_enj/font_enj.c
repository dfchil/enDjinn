#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION
#include "../../include/enDjinn/enj_font_types.h"
#include "../stb/stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */

static int verbose_flag = 0;
#define NUM_GLYPHS '~' - '!' + 1
typedef struct {
  char glyph;
  struct {
    int start;
    int end;
  } trim;
  int width;
  uint8_t *data;
  enj_glyph_offset_t *offset_info;
} glyph_rendering_t;

int read_font_file(const char *path, uint8_t **out_buffer) {
  FILE *fontFile = fopen(path, "rb");
  if (!fontFile) {
    printf("failed to open font file: %s\n", path);
    return 0;
  }

  fseek(fontFile, 0, SEEK_END);
  long size = ftell(fontFile);  /* how long is the file ? */
  fseek(fontFile, 0, SEEK_SET); /* reset */

  uint8_t *fontBuffer = malloc(size);
  if (!fontBuffer) {
    printf("failed to allocate font buffer\n");
    fclose(fontFile);
    return 0;
  }

  if (!fread(fontBuffer, size, 1, fontFile)) {
    printf("failed to read font file\n");
    free(fontBuffer);
    fclose(fontFile);
    return 0;
  }
  fclose(fontFile);

  *out_buffer = fontBuffer;
  return 1;
}

int calculate_sheet_sizes(glyph_rendering_t renderings[NUM_GLYPHS],
                          int line_height, int *out_width, int *out_height) {
  int sheet_width = 64;
  int sheet_height = 64;
  int found_sizes = 0;

  while (!found_sizes && sheet_width < 2048 && sheet_height < 2048) {
    if (verbose_flag) {
      printf("trying sheet_height: %d, sheet_width: %d\n", sheet_height,
             sheet_width);
    }

    int x_progression = 0;
    int line = 0;
    // calculate bitmap size
    char glyph = '!';
    for (int glyph_index = 0; glyph <= '~'; ++glyph, ++glyph_index) {
      if (x_progression + renderings[glyph_index].width > sheet_width) {
        x_progression = 0;
        ++line;
      }
      if ((line + 1) * line_height > sheet_height) {
        if (sheet_height > sheet_width)
          sheet_width <<= 1;
        else
          sheet_height <<= 1;
        break;
      }
      renderings[glyph_index].offset_info->line = (uint16_t)line;
      renderings[glyph_index].offset_info->x_min = (uint16_t)(x_progression);
      x_progression += renderings[glyph_index].width;
    }
    if (glyph >= '~') {
      if (verbose_flag) {
        printf("final sheet_height: %d, sheet_width: %d\n", sheet_height,
               sheet_width);
      }
      found_sizes = 1;

      renderings[NUM_GLYPHS].offset_info->line = (uint16_t)line;
      renderings[NUM_GLYPHS].offset_info->x_min = (uint16_t)(x_progression);
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

int generate_glyph(glyph_rendering_t *rendering, stbtt_fontinfo *info,
                   int line_height, float scale) {
  size_t ext_buffer_width = line_height * 2; // buffer width
  size_t internal_buffer_width = line_height * 3;
  size_t internal_buffer_height = line_height * 2;
  char glyph = rendering->glyph;

  rendering->trim.start = 0;
  rendering->trim.end = ext_buffer_width;

  uint8_t local_buffer[internal_buffer_height * internal_buffer_width];
  // clear buffer
  for (size_t i = 0; i < internal_buffer_height * internal_buffer_width; i++) {
    local_buffer[i] = 0;
  }
  int advance_x, lsb;
  stbtt_GetCodepointHMetrics(info, glyph, &advance_x, &lsb);

  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);
  // /* calculate font scaling */
  ascent = roundf(ascent * scale);
  descent = roundf(descent * scale);
  lsb = roundf(lsb * scale);

  /* get bounding box for character (may be trim to account for chars
   * that dip above or below the line) */
  int c_x1, c_y1, c_x2, c_y2;
  stbtt_GetCodepointBitmapBox(info, glyph, scale, scale, &c_x1, &c_y1, &c_x2,
                              &c_y2);

  size_t int_buf_offset_x = internal_buffer_width >> 2;
  size_t int_buf_offset_y = internal_buffer_height >> 2;

  size_t byteOffset =
      int_buf_offset_x + (int_buf_offset_y * internal_buffer_width) + lsb +
      MAX(ascent + c_y1, 0) * internal_buffer_width - MIN(c_x1, 0);

  /* render character (stride and trim is important here) */
  stbtt_MakeCodepointBitmap(info, local_buffer + byteOffset, c_x2 - c_x1,
                            c_y2 - c_y1, internal_buffer_width, scale, scale,
                            glyph);
  // trim away empty colums
  for (int xp = int_buf_offset_x; xp < ext_buffer_width; xp++) {
    for (int yp = int_buf_offset_y; yp < int_buf_offset_y + line_height; yp++) {
      if (local_buffer[yp * internal_buffer_width + xp] != 0) {
        rendering->trim.start = xp - int_buf_offset_x;
        xp = internal_buffer_width; // break outer loop
        break;
      }
    }
  }
  for (int xp = ext_buffer_width + int_buf_offset_x; xp >= int_buf_offset_x;
       xp--) {
    for (int yp = int_buf_offset_y; yp < int_buf_offset_y + line_height; yp++) {
      if (local_buffer[yp * internal_buffer_width + xp] != 0) {
        rendering->trim.end = xp + 1 - int_buf_offset_x;
        xp = int_buf_offset_x; // break outer loop
        break;
      }
    }
  }
  // copy to external buffer
  for (int yp = 0; yp < line_height; yp++) {
    for (int xp = rendering->trim.start; xp < rendering->trim.end; xp++) {
      int y_src = yp + int_buf_offset_y;
      int x_src = xp + int_buf_offset_x;
      rendering->data[yp * ext_buffer_width + xp] =
          local_buffer[y_src * internal_buffer_width + x_src];
    }
  }
  rendering->width = rendering->trim.end - rendering->trim.start;
  return 1;
}

/**
 * Generate an enDjinn font file from a TTF font
 * @param line_height Line height in pixels
 * @param font_path Path to TTF font file
 * @param output_path Path to output enDjinn font file
 * @param png_control Optional path to output PNG file for debugging
 * @return 0 on success, -1 on failure
 */
int font_genenerator(int line_height, char *font_path, char *output_path,
                     char *png_control) {
  enj_font_header_t header = {0};

  /* prepare font */
  uint8_t *fontBuffer = NULL;
  if (!read_font_file(font_path, &fontBuffer)) {
    return -1;
  }
  stbtt_fontinfo info;
  if (!stbtt_InitFont(&info, fontBuffer, 0)) {
    printf("failed\n");
  }
  // int ascent, descent, lineGap;
  // stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
  // /* calculate font scaling */
  float scale = stbtt_ScaleForPixelHeight(&info, line_height);
  // ascent = roundf(ascent * scale);
  // descent = roundf(descent * scale);
  glyph_rendering_t renderings[NUM_GLYPHS + 1];
  uint8_t *render_buffer =
      calloc(NUM_GLYPHS + 1, line_height * line_height * 2);
  for (int i = 0; i <= NUM_GLYPHS; ++i) {
    renderings[i].glyph = (char)(i + '!');
    renderings[i].data = render_buffer + (i * line_height * line_height * 2);
    renderings[i].offset_info = &header.glyph_endings[i];
    generate_glyph(&renderings[i], &info, line_height, scale);
  }

  int sheet_width;
  int sheet_height;
  if (!calculate_sheet_sizes(renderings, line_height, &sheet_width,
                             &sheet_height)) {
    return -1;
  }
  uint8_t *bitmap = calloc(1, sheet_height * sheet_width * sizeof(uint8_t));
  for (char glyph = 0; glyph < NUM_GLYPHS; ++glyph) {
    int x_min = header.glyph_endings[glyph].x_min;
    int y_min = header.glyph_endings[glyph].line * line_height;

    int glyph_width = header.glyph_endings[glyph + 1].x_min - x_min;
    for (int yp = 0; yp < line_height; yp++) {
      for (int xp = 0; xp < renderings[glyph].width; xp++) {
        uint8_t v1 =
            renderings[glyph].data[yp * line_height * 2 +
                                   (xp + renderings[glyph].trim.start)];
        bitmap[(y_min + yp) * sheet_width + (x_min + xp)] = v1;
      }
    }
  }

  // 4 bits per pixel to PVR4bpp
  uint8_t *pvrout =
      calloc(1, (sheet_height * sheet_width * sizeof(uint8_t)) >> 1);

  for (int i = 0; i < sheet_height * sheet_width; i += 1) {
    uint8_t v1 = bitmap[i];
    uint8_t curcol = ((v1 * 15 + 135) >> 4) & 0xF0;
    bitmap[i] = curcol;
    pvrout[i >> 1] |= i & 1 ? (curcol & 0xF0) : (curcol >> 4);
  }

  /* save out a 1 channel image */
  if (png_control) {
    stbi_write_png(png_control, sheet_width, sheet_height, 1, bitmap,
                   sheet_width);
  }

  /* write out enj font file */
  header.log2width = 5; // start at 32
  while ((1 << header.log2width) < sheet_width) {
    header.log2width++;
  };
  header.log2height = header.log2width; // height is always >= width
  while ((1 << header.log2height) < sheet_height) {
    header.log2height++;
  };
  header.line_height = line_height;

  FILE *outFile = fopen(output_path, "wb");
  fwrite(&header, 1, sizeof(enj_font_header_t), outFile);
  fwrite(pvrout, 1, (sheet_height * sheet_width) / 2, outFile);
  fclose(outFile);

  free(render_buffer);
  free(fontBuffer);
  free(bitmap);
  free(pvrout);

  return 0;
}

int main(int argc, char *argv[]) {
  char *font_path = NULL;
  char *output_path = NULL;
  char *png_control = NULL;
  int line_height = -1;

  const char *usage =
      "Usage:\n\t%s --lineheight <line height> --input <font file> "
      "--output <output file> [--png_control] [--verbose]\n\n";

  static struct option long_options[] = {
      /* These options set a flag. */
      {"verbose", no_argument, &verbose_flag, 1},
      /* These options don’t set a flag.
         We distinguish them by their indices. */
      {"lineheight", required_argument, 0, 'l'},
      {"input", required_argument, 0, 'i'},
      {"output", required_argument, 0, 'o'},
      {"png_control", optional_argument, 0, 'p'},
      {0, 0, 0, 0}};

  int sucess = 1;
  char opt;
  int option_index = 0;

  while ((opt = getopt_long(argc, argv, "l:i:o:vp:d", long_options,
                            &option_index)) != -1) {
    switch (opt) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
        break;
      printf("option %s", long_options[option_index].name);
      if (optarg)
        printf(" with arg %s", optarg);
      printf("\n");
      break;
    case 'l':
      line_height = atoi(optarg);
      break;
    case 'i':
      font_path = optarg;
      break;
    case 'o':
      output_path = optarg;
      break;
    case 'p':
      png_control = optarg;
      break;
    case 'd':
      // debug_flag = 1;
      break;
    default:
      fprintf(stderr, "Unknown option: %c\n", opt);
      sucess = 0;
    }
  }
  if (line_height <= 0 || !font_path || !output_path) {
    if (line_height == -1) {
      fprintf(stderr, "No line height specified\n");
    }
    sucess = 0;
  }
  if (font_path == NULL) {
    fprintf(stderr, "No font path specified\n");
    sucess = 0;
  }
  if (output_path == NULL) {
    fprintf(stderr, "No output path specified\n");
    sucess = 0;
  }
  if (!sucess) {
    fprintf(stderr, usage, argv[0]);
    return -1;
  }
  return font_genenerator(line_height, font_path, output_path, png_control);
}