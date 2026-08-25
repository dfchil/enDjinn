#include <enDjinn/enj_enDjinn.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

typedef struct {
  char fourcc[4];
  uint32_t colors;
} enj_palette_header_t;

_Static_assert(sizeof(enj_palette_header_t) == 8,
               "DPAL headers are always 8 bytes");

int enj_texture_load_blob(const void *data, enj_texture_info_t *texinfo) {
  if (data == NULL || texinfo == NULL) {
    return 0;
  }

  enj_texture_info_t parsed = {0};
  memcpy(&parsed.hdr, data, sizeof(dt_header_t));

  if (memcmp(parsed.hdr.fourcc, "DcTx", 4) != 0) {
    ENJ_DEBUG_PRINT("Error: blob is not a valid DcTx texture!\n");
    return 0;
  }
  size_t metadata_size = ((size_t)1 + parsed.hdr.header_size) << 5;
  if (parsed.hdr.chunk_size <= metadata_size) {
    ENJ_DEBUG_PRINT("Error: DcTx texture has an invalid chunk size!\n");
    return 0;
  }
  size_t tdatasize = parsed.hdr.chunk_size - metadata_size;

  parsed.flags.compressed = fDtIsCompressed(&parsed.hdr);
  parsed.flags.mipmapped = fDtIsMipmapped(&parsed.hdr);
  parsed.flags.palettised = fDtIsPalettized(&parsed.hdr);
  parsed.flags.num_palette_colors = fDtGetColorsUsed(&parsed.hdr);

  if (parsed.flags.palettised) {
    parsed.flags.palette_format = parsed.flags.num_palette_colors == 16
                                      ? PVR_PAL_ARGB4444
                                      : PVR_PAL_ARGB8888;
  } else {
    parsed.flags.palette_format = 0;
  }

  parsed.flags.strided = fDtIsStrided(&parsed.hdr);
  parsed.flags.twiddled = fDtIsTwiddled(&parsed.hdr);
  parsed.width = fDtGetPvrWidth(&parsed.hdr);
  parsed.height = fDtGetPvrHeight(&parsed.hdr);

  parsed.pvrformat = parsed.hdr.pvr_type & 0xFFC00000;

  parsed.ptr = pvr_mem_malloc(tdatasize);
  if (parsed.ptr == NULL) {
    printf("Error: pvr_mem_malloc failed\n");
    return 0;
  }
  pvr_txr_load((const uint8_t *)data + metadata_size, parsed.ptr,
               tdatasize);
  parsed.flags.loaded = 1;
  *texinfo = parsed;
  return 1;
}

int enj_texture_load_file(const char *filename, enj_texture_info_t *texinfo) {
  if (filename == NULL || texinfo == NULL) {
    return 0;
  }
  int success = 1;
  void *buffer = NULL;
  FILE *file = NULL;
  dt_header_t header;
  do {
    file = fopen(filename, "rb");
    if (!file) {
      printf("Error opening file %s: %s\n", filename, strerror(errno));
      success = 0;
      break;
    }

    if (fread(&header, sizeof(header), 1, file) != 1) {
      printf("Error reading header from file %s\n", filename);
      success = 0;
      break;
    }
    if (memcmp(header.fourcc, "DcTx", 4) != 0) {
      printf("Error: not valid DcTx data in file %s\n", filename);
      success = 0;
      break;
    }

    size_t metadata_size = ((size_t)1 + header.header_size) << 5;
    if (header.chunk_size <= metadata_size) {
      printf("Error: invalid DcTx chunk size in file %s\n", filename);
      success = 0;
      break;
    }
    size_t tdatasize = header.chunk_size - metadata_size;

    buffer = memalign(32, header.chunk_size);
    if (!buffer) {
      printf("Error allocating memory for texture data from file %s\n",
             filename);
      success = 0;
      break;
    }
    memcpy(buffer, &header, sizeof(header));
    if (fseek(file, (long)metadata_size, SEEK_SET) != 0 ||
        fread((uint8_t *)buffer + metadata_size, tdatasize, 1, file) != 1) {
      printf("Error reading texture data from file %s\n", filename);
      success = 0;
      break;
    }
    success = enj_texture_load_blob(buffer, texinfo);
  } while (0);

  if (buffer != NULL) {
    free(buffer);
  }
  if (file != NULL) {
    fclose(file);
  }
  return success;
}

int enj_texture_load_palette_blob(const void *raw_data, int fmt,
                                  size_t offset) {
  if (raw_data == NULL) {
    return 0;
  }
  if (fmt != PVR_PAL_ARGB8888 && fmt != PVR_PAL_ARGB4444 &&
      fmt != PVR_PAL_RGB565 && fmt != PVR_PAL_ARGB1555) {
    ENJ_DEBUG_PRINT("Error: unsupported palette format\n");
    return 0;
  }
  enj_palette_header_t palette_hdr;
  memcpy(&palette_hdr, raw_data, sizeof(palette_hdr));
  if (memcmp(palette_hdr.fourcc, "DPAL", 4) != 0) {
    printf("Error: not valid DPAL data\n");
    return 0;
  }
  if (palette_hdr.colors == 0 || palette_hdr.colors > 1024 ||
      offset > 1024 - palette_hdr.colors) {
    ENJ_DEBUG_PRINT("Error: palette range exceeds PVR palette RAM\n");
    return 0;
  }

  uint32_t *colors = (uint32_t *)((char *)raw_data + sizeof(palette_hdr));

  pvr_set_pal_format(fmt);
  for (size_t i = 0; i < palette_hdr.colors; i++) {
    uint32_t color = colors[i]; // format 0xAARRGGBB
    switch (fmt) {
    case PVR_PAL_ARGB8888:
      break;
    case PVR_PAL_ARGB4444:
      color = ((color & 0xF0000000) >> 16 | (color & 0x00F00000) >> 12) |
              ((color & 0x0000F000) >> 8) | ((color & 0x000000F0) >> 4);
      break;
    case PVR_PAL_RGB565:
      color = ((color & 0x00F80000) >> 8) | ((color & 0x0000FC00) >> 5) |
              ((color & 0x000000F8) >> 3);
      break;
    case PVR_PAL_ARGB1555:
      color = ((color & 0x80000000) >> 16) | ((color & 0x00F80000) >> 9) |
              ((color & 0x0000F800) >> 6) | ((color & 0x000000F8) >> 3);
      break;
    default:
      break;
    }
    pvr_set_pal_entry(i + offset, color);
  }
  return 1;
}

int enj_texture_bind_palette(enj_texture_info_t *texinfo, size_t palette_offset) {
  if (texinfo == NULL) {
    return 0;
  }
  if (!texinfo->flags.palettised) {
    ENJ_DEBUG_PRINT("Error: texture is not palettised!\n");
    return 0;
  }
  size_t pal_num = texinfo->flags.palette_format == PVR_PAL_ARGB8888
                       ? palette_offset >> 8
                       : palette_offset >> 4;

  texinfo->pvrformat |=
      (pal_num << (texinfo->flags.palette_format == PVR_PAL_ARGB8888 ? 25 : 21));
  texinfo->flags.palette_position = palette_offset;
  return 1;
}

int enj_texture_load_palette_file(const char *filename, int fmt,
                                  size_t offset) {
  if (filename == NULL) {
    return 0;
  }
  int success = 1;
  FILE *file = NULL;
  void *raw_data = NULL;
  do {
    file = fopen(filename, "rb");
    if (!file) {
      printf("Error opening palette file %s: %s\n", filename, strerror(errno));
      success = 0;
      break;
    }
    enj_palette_header_t palette_hdr;
    if (fread(&palette_hdr, sizeof(palette_hdr), 1, file) != 1) {
      printf("Error reading palette header from file %s\n", filename);
      success = 0;
      break;
    }
    if (memcmp(palette_hdr.fourcc, "DPAL", 4) != 0) {
      printf("Error: not valid DPAL data in file %s\n", filename);
      success = 0;
      break;
    }
    if (palette_hdr.colors == 0 || palette_hdr.colors > 1024 ||
        offset > 1024 - palette_hdr.colors) {
      printf("Error: palette range exceeds PVR palette RAM in file %s\n",
             filename);
      success = 0;
      break;
    }
    raw_data = memalign(32, palette_hdr.colors * sizeof(uint32_t) +
                                sizeof(palette_hdr));
    if (!raw_data) {
      printf("Error allocating memory for palette colors from file %s\n",
             filename);
      success = 0;
      break;
    }
    memcpy(raw_data, &palette_hdr, sizeof(palette_hdr));
    if (fread((char *)raw_data + sizeof(palette_hdr),
              palette_hdr.colors * sizeof(uint32_t), 1, file) != 1) {
      printf("Error reading palette colors from file %s\n", filename);
      success = 0;
      break;
    }
    success = enj_texture_load_palette_blob(raw_data, fmt, offset);
  } while (0);

  if (file != NULL) {
    fclose(file);
  }
  if (raw_data != NULL) {
    free(raw_data);
  }
  return success;
}

int enj_texture_unload(enj_texture_info_t *texinfo) {
  if (texinfo == NULL) {
    return 0;
  }
  if (texinfo->ptr != NULL) {
    pvr_mem_free(texinfo->ptr);
    texinfo->ptr = NULL;
    texinfo->flags.loaded = 0;
    return 1;
  }
  return 0;
}
