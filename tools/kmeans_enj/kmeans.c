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

void kmeans16colors(const uint8_t* input, uint8_t* output,
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
    return 0;
}
