#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */


typedef struct {
    uint16_t dimensions;
    uint16_t line_height;
    uint16_t glyph_starts[127-33];
    uint32_t palette[16];
    unsigned char bitmap[]; /* 4bpp alpha */
}
enj_font_t;

int main(int argc, const char* argv[]) {
    /* load font file */
    long size;
    unsigned char* fontBuffer;
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

    uint16_t glyph_starts[127-33] = {0};

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

            int byteOffset =
                x + roundf(lsb * scale) + (y * width * 2) + (line * l_h * width);

            if (byteOffset > width * width) {
                break;
            }

            /* advance x */
            x += roundf(ax * scale) + 2;
            // int kern = stbtt_GetCodepointKernAdvance(&info, glyph, glyph + 1);
            // printf ("kern %d\n", kern);
            // x += roundf(kern * scale);

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
    unsigned char* bitmap = calloc(b_d * b_d, sizeof(unsigned char));

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
        glyph_starts[glyph - 33] = x+ line * b_d; 
        printf("gw %d\n", x+ line * b_d);
    }

    // make it 16 colors with alpha
    unsigned char* output = calloc(b_d * b_d * 4, sizeof(unsigned char));
    for (int i = 0; i < b_d * b_d; ++i) {
        unsigned char v = bitmap[i];
        output[i * 4 + 0] = 255;
        output[i * 4 + 1] = 255;
        output[i * 4 + 2] = 255;
        output[i * 4 + 3] = ((v * 15 + 135) >> 4) & 0xf0; /* 16 levels of alpha * 16 colors */
    }

    /* save out a 1 channel image */
    stbi_write_png("out.png", b_d, b_d, 4, output, b_d * 4);

    /*
     Note that this example writes each character directly into the target image
     buffer. The "right thing" to do for fonts that have overlapping characters
     is MakeCodepointBitmap to a temporary buffer and then alpha blend that onto
     the target image. See the stb_truetype.h header for more info.
    */

    free(fontBuffer);
    free(bitmap);

    return 0;
}
