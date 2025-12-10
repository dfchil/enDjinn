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
static int horizontal_first = 0;

#define NUM_GLYPHS '~' - '!' + 1
typedef struct {
    char glyph;
    struct {
        int start;
        int end;
    } trim;
    int width;
    uint8_t* data;
    // enj_glyph_offset_t *offset_info;
} glyph_rendering_t;

uint8_t* read_font_file(const char* path) {
    FILE* fontFile = fopen(path, "rb");
    if (!fontFile) {
        printf("failed to open font file: %s\n", path);
        return NULL;
    }
    fseek(fontFile, 0, SEEK_END);
    long size = ftell(fontFile);  /* how long is the file ? */
    fseek(fontFile, 0, SEEK_SET); /* reset */

    uint8_t* fontBuffer = malloc(size);
    if (!fontBuffer) {
        printf("failed to allocate font buffer\n");
        fclose(fontFile);
        return NULL;
    }
    if (!fread(fontBuffer, size, 1, fontFile)) {
        printf("failed to read font file\n");
        free(fontBuffer);
        fclose(fontFile);
        return NULL;
    }
    fclose(fontFile);
    return fontBuffer;
}

int calculate_sheet_sizes(glyph_rendering_t renderings[NUM_GLYPHS],
                          enj_font_header_t* enj_font, int* out_width,
                          int* out_height) {
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
        char glyph = '!';

        // set first glyph
        for (int glyph_index = 0; glyph <= '~'; ++glyph, ++glyph_index) {
            if (!enj_font->glyph_endings[glyph_index].available) {
                enj_font->glyph_endings[glyph_index].x_min =
                    enj_font->glyph_endings[glyph_index - 1].x_min;
                enj_font->glyph_endings[glyph_index].line =
                    enj_font->glyph_endings[glyph_index - 1].line;
                continue;
            }
            enj_font->glyph_endings[glyph_index].x_min =
                (uint16_t)(x_progression);
            x_progression += renderings[glyph_index].width;
            if (x_progression > sheet_width) {
                // enj_font->glyph_endings[glyph_index].x_min = 0;
                x_progression = renderings[glyph_index].width;
                ++line;
            }
            enj_font->glyph_endings[glyph_index].line = (uint16_t)line;

            if ((line + 1) * enj_font->line_height > sheet_height) {
                if (horizontal_first) {
                    if (sheet_width > sheet_height) {
                        sheet_height <<= 1;
                    } else {
                        sheet_width <<= 1;
                    }
                } else {
                    if (sheet_height > sheet_width) {
                        sheet_width <<= 1;
                    } else {
                        sheet_height <<= 1;
                    }
                }
                break;
            }
        }
        if (glyph >= '~') {
            if (verbose_flag) {
                printf("final sheet_height: %d, sheet_width: %d\n",
                       sheet_height, sheet_width);
            }
            found_sizes = 1;

            enj_font->glyph_endings[NUM_GLYPHS].line = (uint16_t)line;
            enj_font->glyph_endings[NUM_GLYPHS].x_min =
                (uint16_t)(x_progression);
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

int generate_glyph(glyph_rendering_t* rendering, stbtt_fontinfo* info,
                   int line_height, float scale) {
    size_t ext_buffer_width = line_height * 2;  // buffer width
    size_t internal_buffer_width = line_height * 3;
    size_t internal_buffer_height = line_height * 2;
    char glyph = rendering->glyph;

    rendering->trim.start = 0;
    rendering->trim.end = ext_buffer_width;

    uint8_t local_buffer[internal_buffer_height * internal_buffer_width];
    memset(local_buffer, 0, internal_buffer_height * internal_buffer_width);
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
        for (int yp = int_buf_offset_y; yp < int_buf_offset_y + line_height;
             yp++) {
            if (local_buffer[yp * internal_buffer_width + xp] != 0) {
                rendering->trim.start = xp - int_buf_offset_x;
                xp = internal_buffer_width;  // break outer loop
                break;
            }
        }
    }
    for (int xp = ext_buffer_width + int_buf_offset_x; xp >= int_buf_offset_x;
         xp--) {
        for (int yp = int_buf_offset_y; yp < int_buf_offset_y + line_height;
             yp++) {
            if (local_buffer[yp * internal_buffer_width + xp] != 0) {
                rendering->trim.end = xp + 1 - int_buf_offset_x;
                xp = int_buf_offset_x;  // break outer loop
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
int font_genenerator(int line_height, char* font_path, char* output_path,
                     char* png_control, char* special_chars,
                     char* exclude_chars) {
    enj_font_header_t header = {0};
    header.version.major = 0;
    header.version.minor = 1;
    header.version.patch = 1;
    printf("version %d.%d.%d\n", header.version.major, header.version.minor,
           header.version.patch);
    printf("sizeof header: %lu\n", sizeof(enj_font_header_t));

    header.line_height = line_height;

    /* prepare font */
    uint8_t* fontBuffer = read_font_file(font_path);
    if (!fontBuffer) {
        return -1;
    }
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, fontBuffer, 0)) {
        printf("failed\n");
    }
    float scale = stbtt_ScaleForPixelHeight(&info, line_height);
    glyph_rendering_t renderings[NUM_GLYPHS + 1];
    glyph_rendering_t special_renderings[sizeof(header.special_glyphs) /
                                         sizeof(enj_glyph_offset_t)];
    uint8_t* render_buffer =
        calloc(NUM_GLYPHS + 1 +
                   (sizeof(special_renderings) / sizeof(glyph_rendering_t)),
               line_height * line_height * 2);
    if (!render_buffer) {
        printf("failed to allocate render buffer\n");
        free(fontBuffer);
        return -1;
    }
    for (int glyph = 0; glyph < NUM_GLYPHS; ++glyph) {
        renderings[glyph].glyph = (char)(glyph + '!');
        renderings[glyph].data =
            render_buffer + (glyph * line_height * line_height * 2);
        if (exclude_chars) {
            char g = (char)(glyph + '!');
            char* ex = exclude_chars;
            int found = 0;
            while (*ex != '\0') {
                if (*ex == g) {
                    found = 1;
                    break;
                }
                ex++;
            }
            if (found) {
                if (verbose_flag) {
                    printf(" excluding glyph '%c'\n", g);
                }
                header.glyph_endings[glyph].available = 0;
                renderings[glyph].width = 0;
                renderings[glyph].data = NULL;
                renderings[glyph].trim.start = 0;
                renderings[glyph].trim.end = 0;
                renderings[glyph].width = 0;
                continue;
            }
        }
        header.glyph_endings[glyph].available = 1;
        generate_glyph(renderings + glyph, &info, line_height, scale);
    }

    int sheet_width;
    int sheet_height;
    if (!calculate_sheet_sizes(renderings, &header, &sheet_width,
                               &sheet_height)) {
        return -1;
    }
    uint8_t* bitmap = calloc(1, sheet_height * sheet_width * sizeof(uint8_t));
    for (char glyph = 0; glyph < NUM_GLYPHS; ++glyph) {
        if (!header.glyph_endings[glyph].available) {
            continue;
        }
        int x_min = header.glyph_endings[glyph].x_min;
        int y_min = header.glyph_endings[glyph].line * line_height;
        if (header.glyph_endings[glyph].line >
            header.glyph_endings[glyph - 1].line) {
            x_min = 0;
        }
        for (int yp = 0; yp < line_height; yp++) {
            for (int xp = 0; xp < renderings[glyph].width; xp++) {
                uint8_t v1 =
                    renderings[glyph].data[yp * line_height * 2 +
                                           (xp + renderings[glyph].trim.start)];
                bitmap[(y_min + yp) * sheet_width + (x_min + xp)] = v1;
            }
        }
    }
    // if (special_chars) {
    //     char* sc = special_chars;
    //     for (int special_index = 0;
    //          *sc != '\0' && special_index < (sizeof(special_renderings) /
    //                                          sizeof(enj_glyph_offset_t)),
    //              i++, sc++) {
    //         special_renderings[special_index].glyph = *sc;
    //         special_renderings[special_index].data =
    //             render_buffer +
    //             ((NUM_GLYPHS + special_index) * line_height * line_height * 2);
    //         generate_glyph(special_renderings + special_index, &info,
    //                        line_height, scale);
    //         // copy to bitmap
    //         int x_min = header.glyph_endings[NUM_GLYPHS].x_min;
    //         int y_min = header.glyph_endings[NUM_GLYPHS].line * line_height;
    //         if (header.special_glyphs[NUM_GLYPHS].line >
    //             header.glyph_endings[NUM_GLYPHS - 1].line) {
    //             x_min = 0;  // start of new line
    //         }
    //     }
    // }

    // 4 bits per pixel to PVR4bpp
    uint8_t* pvrout =
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
    header.log2width = 5;  // start at 32
    while ((1 << header.log2width) < sheet_width) {
        header.log2width++;
    };
    header.log2height = 5; 
    while ((1 << header.log2height) < sheet_height) {
        header.log2height++;
    };

    if (verbose_flag) {
        printf("font header info:\n");
        printf(" log2width: %d\n", header.log2width);
        printf(" log2height: %d\n", header.log2height);
        printf(" line_height: %d\n", header.line_height);

        for (char glyph = '!'; glyph <= '~'; ++glyph) {
            int glyph_index = (uint32_t)glyph - '!';
            enj_glyph_offset_t glyph_start = header.glyph_endings[glyph_index];
            enj_glyph_offset_t glyph_end =
                header.glyph_endings[glyph_index + 1];
            printf(" Glyph '%c': %d:%d to %d:%d\n", glyph, glyph_start.line,
                   glyph_start.x_min, glyph_end.line, glyph_end.x_min);
        }
    }

    FILE* outFile = fopen(output_path, "wb");
    fwrite(&header, 1, sizeof(enj_font_header_t), outFile);
    fwrite(pvrout, 1, (sheet_height * sheet_width) / 2, outFile);
    fclose(outFile);
    free(render_buffer);
    free(fontBuffer);
    free(bitmap);
    free(pvrout);

    return 0;
}

int main(int argc, char* argv[]) {
    char* font_path = NULL;
    char* output_path = NULL;
    char* png_control = NULL;
    char* special_chars = NULL;
    char* exclude_chars = NULL;
    int line_height = -1;

    const char* usage =
        "Usage:\n\t%s --lineheight <line height> --input <font file> "
        "\t\t--output <output file> [--png_control <ctrl image  >] "
        "[--verbose]\n"
        "--special  "
        "\n\n";

    static struct option long_options[] = {
        /* These options set a flag. */
        {"verbose", no_argument, &verbose_flag, 1},
        /* These options don’t set a flag.
           We distinguish them by their indices. */
        {"lineheight", required_argument, 0, 'l'},
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"png_control", optional_argument, 0, 'p'},
        {"special", optional_argument, 0, 's'},
        {"exclude", optional_argument, 0, 'x'},

        {0, 0, 0, 0}};

    int sucess = 1;
    char opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "l:i:o:p:x:s:vdh", long_options,
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
            case 'd':
            case 'v':
                verbose_flag = 1;
                break;
            case 'x':
                printf("eXclude characters %s", optarg);
                exclude_chars = optarg;
                break;
            case 's':
                printf("additional charachters %s", optarg);
            case '?':
                printf("help text!\n");
            case 'h':
                horizontal_first = 1;
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
    return font_genenerator(line_height, font_path, output_path, png_control,
                            special_chars, exclude_chars);
}