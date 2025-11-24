#include <enDjinn/enj_enDjinn.h>
#include <errno.h>
#include <kos/fs.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

// notice KallistiOS runs in little-endian mode
static const uint32_t DcTx_chksm = (uint32_t)'D' << 0 | (uint32_t)'c' << 8 |
                                   (uint32_t)'T' << 16 | (uint32_t)'x' << 24;
static const uint32_t DPAL_chksm = (uint32_t)'D' << 0 | (uint32_t)'P' << 8 |
                                   (uint32_t)'A' << 16 | (uint32_t)'L' << 24;

int enj_pvrtex_load(const char* filename, enj_dttex_info_t* texinfo) {
    int success = 1;
    file_t fp = -1;
    
    do {
        ENJ_DEBUG_PRINT("Loading texture from file: %s\n", filename);
        fp = fs_open(filename, O_RDONLY);
        if (fp == -1) {
            ENJ_DEBUG_PRINT("Error: fopen %s failed, %s\n", filename,
                            strerror(errno));
            success = 0;
            break;
        }
        size_t bread = fs_read(fp, texinfo, sizeof(dt_header_t));
        if (bread != sizeof(dt_header_t)) {
            ENJ_DEBUG_PRINT("Error: fread failed for %s\n", filename);
            success = 0;
            break;
        }

        if (*((uint32_t*)&texinfo->hdr.fourcc) != DcTx_chksm) {
            ENJ_DEBUG_PRINT("Error: %s is not a valid DcTx file\n", filename);
            success = 0;
            break;
        }
        size_t tdatasize =
            texinfo->hdr.chunk_size - ((1 + texinfo->hdr.header_size) << 5);
        
        dt_header_t temp_hdr = texinfo->hdr;

        texinfo->flags.compressed = fDtIsCompressed(&temp_hdr);
        texinfo->flags.mipmapped = fDtIsMipmapped(&temp_hdr);
        texinfo->flags.palettised = fDtIsPalettized(&temp_hdr);
        texinfo->flags.num_palette_colors = fDtGetColorsUsed(&temp_hdr);

        if (texinfo->flags.palettised) {
            texinfo->flags.palette_format =
                texinfo->flags.num_palette_colors == 16 ? PVR_PAL_ARGB4444
                                                        : PVR_PAL_ARGB8888;
        } else {
            texinfo->flags.palette_format = 0;
        }

        texinfo->flags.strided = fDtIsStrided(&temp_hdr);
        texinfo->flags.twiddled = fDtIsTwiddled(&temp_hdr);
        texinfo->width = fDtGetPvrWidth(&temp_hdr);
        texinfo->height = fDtGetPvrHeight(&temp_hdr);

        texinfo->pvrformat = texinfo->hdr.pvr_type & 0xFFC00000;

        void* buffer = memalign(32, tdatasize);
        if (buffer == NULL) {
            ENJ_DEBUG_PRINT("Error: memalign failed\n");
            success = 0;
            break;
        }
        fs_read(fp, buffer, tdatasize);

        texinfo->ptr = pvr_mem_malloc(tdatasize);
        if (texinfo->ptr == NULL) {
            free(buffer);
            ENJ_DEBUG_PRINT("Error: pvr_mem_malloc failed\n");
            success = 0;
            break;
        }
        pvr_txr_load(buffer, texinfo->ptr, tdatasize);
        free(buffer);
    } while (0);

    if (fp != -1) {
        fs_close(fp);
    }
    return success;
}

int enj_pvrtex_load_palette(const char* filename, int fmt, size_t offset) {
    int success = 1;
    struct {
        char fourcc[4];
        size_t colors;
    } palette_hdr;

    file_t fp = -1;
    do {
        fp = fs_open(filename, O_RDONLY);
        if (fp == -1) {
            ENJ_DEBUG_PRINT("Error: fopen %s failed, %s\n", filename,
                            strerror(errno));
            success = 0;
            break;
        }
        fs_read(fp, &palette_hdr, sizeof(palette_hdr));
        if (*((uint32_t*)&palette_hdr.fourcc) != DPAL_chksm) {
            ENJ_DEBUG_PRINT("Error: %s is not a valid DPAL file\n", filename);
            success = 0;
            break;
        }
        uint32_t colors[MIN(palette_hdr.colors, 256)];

        fs_read(fp, &colors, sizeof(uint32_t) * palette_hdr.colors);

        pvr_set_pal_format(fmt);
        for (size_t i = 0; i < palette_hdr.colors; i++) {
            uint32_t color = colors[i];  // format 0xAARRGGBB
            switch (fmt) {
                case PVR_PAL_ARGB8888:
                    break;
                case PVR_PAL_ARGB4444:
                    color = ((color & 0xF0000000) >> 16 |
                             (color & 0x00F00000) >> 12) |
                            ((color & 0x0000F000) >> 8) |
                            ((color & 0x000000F0) >> 4);
                    break;
                case PVR_PAL_RGB565:
                    color = ((color & 0x00F80000) >> 8) |
                            ((color & 0x0000FC00) >> 5) |
                            ((color & 0x000000F8) >> 3);
                    break;
                case PVR_PAL_ARGB1555:
                    color = ((color & 0x80000000) >> 16) |
                            ((color & 0x00F80000) >> 9) |
                            ((color & 0x0000F800) >> 6) |
                            ((color & 0x000000F8) >> 3);
                    break;
                default:
                    break;
            }
            pvr_set_pal_entry(i + offset, color);
        }
    } while (0);

    if (fp != -1) {
        fs_close(fp);
    }
    return success;
}

int enj_pvrtex_unload(enj_dttex_info_t* texinfo) {
    if (texinfo->ptr != NULL) {
        pvr_mem_free(texinfo->ptr);
        texinfo->ptr = NULL;
        return 1;
    }
    return 0;
}
