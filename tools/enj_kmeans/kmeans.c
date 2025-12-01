#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */


typedef struct {
    uint16_t dimensions;
    uint16_t line_height;
    uint16_t glyph_starts[127 - 33];
    uint8_t bitmap[]; /* 4bpp alpha */
} enj_font_t;

static int intsort(const void* p1, const void* p2) {
    uint32_t v1 = *(uint32_t*)p1;
    uint32_t v2 = *(uint32_t*)p2;
    if (v1 < v2)
        return -1;
    else if (v1 > v2)
        return 1;
    else
        return 0;
}

void kmeans256to16colors(const uint8_t* input, uint8_t* output,
                         uint8_t* palette, int width, int height) {
    int iteration_num = 0;
    // Simple uniform quantization for demonstration purposes
    // A real k-means implementation would be more complex
    int converged = 0;

    uint32_t old_palette[16] = {0};
    for (int i = 0; i < 16; ++i) {
        old_palette[i] = (i * 17);  // Simple grayscale palette
    }
    while (!converged) {
        // for (int i = 0; i < 16; ++i) {
        //     printf("palette %d: %d\n", i, old_palette[i]);
        // }
        converged = 1;
        uint32_t new_sums[16] = {0};
        int counts[16] = {0};
        for (int i = 0; i < width * height; ++i) {
            uint8_t v = input[i];
            if (v == 0) {
                output[i] = 0;
                counts[0]++;
                continue;  // transparent
            }
            if (v == 255) {
                counts[15]++;
                output[i] = 15;
                continue;  // opaque
            }
            uint8_t best_index = 0;
            uint8_t best_distance = 255;
            for (int j = 1; j < 15; ++j) {
                uint8_t distance = abs(v - old_palette[j]);
                if (distance < best_distance) {
                    best_distance = distance;
                    best_index = j;
                }
            }
            if (output[i] != best_index) {
                converged = 0;
                output[i] = best_index;
            }
            new_sums[best_index] += v;
            counts[best_index]++;
        }
        uint32_t new_palette[16] = {0};
        new_palette[15] = 255;
        for (int j = 0; j < 16; ++j) {
            printf("color %d: value %d count %d\n", j, new_sums[j] / counts[j],
                   counts[j]);
            if (j == 0 || j == 15) {
                continue;  // transparent or opaque
            }
            if (counts[j] > 0) {
                int mean = new_sums[j] /= counts[j];
                if (mean != old_palette[j]) {
                    converged = 0;
                }
                new_palette[j] = mean;
            } else {
                new_palette[j] = 0;
            }
        }
        for (int j = 0; j < 16; ++j) {
            old_palette[j] = new_palette[j];
        }
        iteration_num++;
        if (iteration_num > 100) {
            break;  // prevent infinite loops
        }
        printf("kmeans iteration %d\n", iteration_num);
    }
    // qsort(old_palette, 16, sizeof(uint32_t), intsort);
    for (int i = 0; i < width * height; ++i) {
        output[i] = old_palette[output[i]];
    }
}

int main(int argc, const char* argv[]) {
    /* load font file */
    long size;
    uint8_t* fontBuffer;
    // FILE* fontFile = fopen("font/DejaVuSans.ttf", "rb");
    FILE* fontFile = fopen("font/cmunrm.ttf", "rb");

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

    int l_h = 64; /* line height */

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

    /* calculate font scaling */
    float scale = stbtt_ScaleForPixelHeight(&info, l_h);

    ascent = roundf(ascent * scale);
    descent = roundf(descent * scale);

    int b_d = -1;
    int x = 0;
    int line = 0;

    uint16_t glyph_starts[127 - 33] = {0};

    for (int width = 64; width <= 1024; width <<= 1) {
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
            stbtt_GetCodepointBitmapBox(&info, glyph, scale, scale, &c_x1,
                                        &c_y1, &c_x2, &c_y2);
            if (x + (ax * scale) > width) {
                x = 0;
                ++line;
            }
            int y = ascent + c_y1;

            int byteOffset = x + roundf(lsb * scale) + (y * width * 2) +
                             (line * l_h * width);

            if (byteOffset > width * width) {
                break;
            }

            /* advance x */
            x += roundf(ax * scale) + 2;
            // int kern = stbtt_GetCodepointKernAdvance(&info, glyph, glyph +
            // 1); printf ("kern %d\n", kern); x += roundf(kern * scale);
        }
        if (glyph == 127) {
            b_d = width;
            break;
        }
    }
    printf("total width: %d\n", b_d);
    if (b_d == -1) {
        printf("bitmap too small\n");
        return 1;
    }

    /* create a bitmap for the phrase */
    uint8_t* bitmap = calloc(b_d * b_d, sizeof(uint8_t));

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
        stbtt_GetCodepointBitmapBox(&info, glyph, scale, scale, &c_x1, &c_y1,
                                    &c_x2, &c_y2);
        if (x + (ax * scale) > b_d) {
            x = 0;
            ++line;
        }

        /* compute y (different characters have different heights) */
        int y = ascent + c_y1;

        /* render character (stride and offset is important here) */
        int byteOffset =
            x + roundf(lsb * scale) + (y * b_d) + (line * l_h * b_d);
        stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1,
                                  c_y2 - c_y1, b_d, scale, scale, glyph);

        // printf("x:%d y:%d (%d..%d x %d..%d) ax:%d lsb:%d\n", x,
        //        y + (line * l_h), c_x1, c_x2, c_y1, c_y2, ax, lsb);

        /* advance x */
        x += roundf(ax * scale) + 2;

        /* add kerning */
        // x += roundf(kern * scale);
        // int kern = stbtt_GetCodepointKernAdvance(&info, glyph, glyph + 1);
        glyph_starts[glyph - 33] = x + line * b_d;
    }

    uint8_t* quantized_bitmap = malloc(b_d * b_d * sizeof(uint8_t));
    uint8_t* palette_indices[16] = {0};
    kmeans256to16colors(bitmap, quantized_bitmap, (uint8_t*)palette_indices,
                        b_d, b_d);
    stbi_write_png("out_q.png", b_d, b_d, 1, quantized_bitmap, b_d);

    // make it 16 colors with alpha
    uint8_t* output = calloc(b_d * b_d, sizeof(uint8_t));
    uint32_t new_palette[16] = {0};
    uint32_t new_count[16] = {0};
    uint32_t other_count[16] = {0};
    uint8_t colors_found = 1;
    for (int i = 0; i < b_d * b_d; i += 1) {
        uint8_t v1 = bitmap[i];
        uint8_t v2 = bitmap[i + 1];

        uint8_t curcol = ((v1 * 15 + 135) >> 4) & 0xF0;
        uint8_t other_index =
            (v1 >> 4) + ((v1 & 0xf0) < 240 && ((v1 & 0x0f) > 7) ? 1 : 0);
        other_count[other_index]++;
        // other_index = v2 >> 4 + ((((v2 & 0xf0) < 240) && (v2 & 0x0f) > 7) ? 1
        // : 0);
        if (curcol == 255) {
            printf("found opaque pixel at %d\n", i);
        }
        if (curcol != 0 && colors_found < 16) {
            for (int j = 1; j < 16; ++j) {
                if (new_palette[j] == curcol) {
                    break;
                }
                if (new_palette[j] == 0) {
                    new_palette[j] = curcol;
                    colors_found++;
                    break;
                }
            }
        }
        // output[i * 4 + 0] = 255;
        // output[i * 4 + 1] = 255;
        // output[i * 4 + 2] = 255;
        output[i] = other_index << 4;
        new_count[curcol >> 4]++;
    }
    // qsort(new_palette, 16, sizeof(uint32_t), intsort);
    for (int i = 0; i < 16; ++i) {
        printf("final palette %d: %d, count: %lu, alt_count %lu\n", i,
               new_palette[i], new_count[i], other_count[i]);
    }

    /* save out a 1 channel image */
    stbi_write_png("out.png", b_d, b_d, 1, output, b_d);

    /*
     Note that this example writes each character directly into the target image
     buffer. The "right thing" to do for fonts that have overlapping characters
     is MakeCodepointBitmap to a temporary buffer and then alpha blend that onto
     the target image. See the stb_truetype.h header for more info.
    */

    free(fontBuffer);
    free(bitmap);
    free(output);
    free(quantized_bitmap);

    return 0;
}
