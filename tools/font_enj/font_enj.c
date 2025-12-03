#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION
#include "../../include/enDjinn/enj_font_types.h"
#include "../stb/stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */

static int verbose_flag = 0;

int calculate_sheet_sizes(int line_height, stbtt_fontinfo* info, int* out_width,
                          int* out_height) {
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
            if (verbose_flag) {
                printf("final sheet_height: %d, sheet_width: %d\n",
                       sheet_height, sheet_width);
            }
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

int read_font_file(const char* path, uint8_t** out_buffer) {
    FILE* fontFile = fopen(path, "rb");
    if (!fontFile) {
        printf("failed to open font file: %s\n", path);
        return 0;
    }

    fseek(fontFile, 0, SEEK_END);
    long size = ftell(fontFile);  /* how long is the file ? */
    fseek(fontFile, 0, SEEK_SET); /* reset */

    uint8_t* fontBuffer = malloc(size);
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

/**
 * Generate an enDjinn font file from a TTF font
 * @param line_height Line height in pixels
 * @param font_path Path to TTF font file
 * @param output_path Path to output enDjinn font file
 * @param png_control Optional path to output PNG file for debugging
 * @return 0 on success, -1 on failure
 */
int font_gen(int line_height, char* font_path, char* output_path,
             char* png_control) {

    printf("png control: %s\n", png_control);

    enj_font_header_t header = {0};

    /* prepare font */
    uint8_t* fontBuffer = NULL;
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
    if (!calculate_sheet_sizes(line_height, &info, &sheet_width,
                               &sheet_height)) {
        return -1;
    }
    uint8_t* bitmap = calloc(1, sheet_height * sheet_width * sizeof(uint8_t));
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
        stbtt_GetCodepointBitmapBox(&info, glyph, scale, scale, &c_x1, &c_y1,
                                    &c_x2, &c_y2);
        if (cur_pos_x + (advance_x * scale) > sheet_width) {
            cur_pos_x = 0;
            ++cur_line;
        }
        /* compute y (different characters have different heights) */
        int y = ascent + c_y1;

        /* render character (stride and offset is important here) */
        int byteOffset = cur_pos_x + roundf(lsb * scale) + (y * sheet_width) +
                         (cur_line * line_height * sheet_width);
        stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1,
                                  c_y2 - c_y1, sheet_width, scale, scale,
                                  glyph);

        /* advance x */
        cur_pos_x += roundf(advance_x * scale);

        /* add kerning */
        int kern = stbtt_GetCodepointKernAdvance(&info, glyph, glyph + 1);
        cur_pos_x += roundf(kern * scale);
        header.glyph_starts[glyph - 33] =
            (enj_glyph_offset_t){.available = 1,
                                 .line = (uint16_t)cur_line,
                                 .offset_end = (uint16_t)(cur_pos_x)};
    }

    // 4 bits per pixel to PVR4bpp
    uint8_t* pvrout =
        calloc(1, (sheet_height * sheet_width * sizeof(uint8_t)) >> 1);

    for (int i = 0; i < sheet_height * sheet_width; i += 1) {
        uint8_t v1 = bitmap[i];
        uint8_t curcol = ((v1 * 15 + 135) >> 4) & 0xF0;
        bitmap[i] = curcol;
        pvrout[i >> 1] |= i & 1 ? (curcol >> 4) : (curcol & 0xF0);
    }

    /* save out a 1 channel image */
    if (png_control) {
        stbi_write_png(png_control, sheet_width, sheet_height, 1, bitmap,
                       sheet_width);
    }

    /* write out enj font file */
    header.log2height = 5;  // start at 32
    while ((1 << header.log2height) < sheet_height) {
        header.log2height++;
    };
    header.log2width = header.log2height;  // width is always >= height
    while ((1 << header.log2width) < sheet_width) {
        header.log2width++;
    };
    if (verbose_flag) {
        printf("final log2width: %d, log2height: %d\n", header.log2width,
               header.log2height);
    }


    header.line_height = line_height;

    FILE* outFile = fopen(output_path, "wb");
    fwrite(&header, 1, sizeof(enj_font_header_t), outFile);
    fwrite(pvrout, 1, (sheet_height * sheet_width) / 2, outFile);
    fclose(outFile);

    free(fontBuffer);
    free(bitmap);
    free(pvrout);

    return 0;
}

int main(int argc, char* argv[]) {
    char* font_path = NULL;
    char* output_path = NULL;
    char* png_control = NULL;
    int line_height = -1;

    const char* usage =
        "Usage: %s --lineheight <line height> --input <font file> "
        "--output <output file> [--png_control] [--verbose]\n";

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

    while ((opt = getopt_long(argc, argv, "l:i:o:vp:", long_options,
                              &option_index)) != -1) {
        switch (opt) {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0) break;
                printf("option %s", long_options[option_index].name);
                if (optarg) printf(" with arg %s", optarg);
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

            default:
                fprintf(stderr, "Unknown option: %c\n", opt);
                sucess = 0;
        }
    }
    if (line_height == -1 || !font_path || !output_path) {
        if (line_height == -1) {
            fprintf(stderr, "No line height specified\n");
            sucess = 0;
        }
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

    printf("sizeof enj_font_header_t: %zu\n", sizeof(enj_font_header_t));

    return font_gen(line_height, font_path, output_path, png_control);
}