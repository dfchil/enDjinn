#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <enDjinn/enj_texture.h>

static pvr_palfmt_t palette_format;
static uint32_t palette_index;
static uint32_t palette_value;

void *memalign(size_t alignment, size_t size) {
  (void)alignment;
  return malloc(size);
}
void *pvr_mem_malloc(size_t size) { return malloc(size); }
void pvr_mem_free(pvr_ptr_t ptr) { free(ptr); }
void pvr_txr_load(const void *src, pvr_ptr_t dst, size_t count) {
  memcpy(dst, src, count);
}
void pvr_set_pal_format(pvr_palfmt_t mode) { palette_format = mode; }
void pvr_set_pal_entry(uint32_t index, uint32_t value) {
  palette_index = index;
  palette_value = value;
}

int main(void) {
  assert(enj_texture_load_blob(NULL, NULL) == 0);
  assert(enj_texture_load_file(NULL, NULL) == 0);
  assert(enj_texture_unload(NULL) == 0);

  struct {
    dt_header_t header;
    uint32_t pixel;
  } texture = {
      .header = {.fourcc = {'D', 'c', 'T', 'x'},
                 .chunk_size = sizeof(dt_header_t) + sizeof(uint32_t),
                 .pvr_type = (FDT_FMT_ARGB1555 << 27)},
      .pixel = 0x12345678u,
  };
  enj_texture_info_t info;
  assert(enj_texture_load_blob(&texture, &info) == 1);
  assert(info.flags.loaded == 1);
  assert(memcmp(info.ptr, &texture.pixel, sizeof(texture.pixel)) == 0);
  assert(enj_texture_unload(&info) == 1);
  assert(info.ptr == NULL && info.flags.loaded == 0);

  struct {
    dt_header_t header;
    uint8_t extension[32];
    uint32_t pixel;
  } extended_texture = {
      .header = {.fourcc = {'D', 'c', 'T', 'x'},
                 .chunk_size = sizeof(dt_header_t) + 32 + sizeof(uint32_t),
                 .header_size = 1,
                 .pvr_type = (FDT_FMT_ARGB1555 << 27)},
      .extension = {0xa5},
      .pixel = 0x89abcdefu,
  };
  assert(enj_texture_load_blob(&extended_texture, &info) == 1);
  assert(memcmp(info.ptr, &extended_texture.pixel,
                sizeof(extended_texture.pixel)) == 0);
  assert(enj_texture_unload(&info) == 1);

  texture.header.chunk_size = sizeof(dt_header_t);
  assert(enj_texture_load_blob(&texture, &info) == 0);

  struct {
    char fourcc[4];
    uint32_t colors;
    uint32_t value;
  } palette = {{'D', 'P', 'A', 'L'}, 1, 0xff112233u};
  assert(enj_texture_load_palette_blob(NULL, PVR_PAL_ARGB8888, 0) == 0);
  assert(enj_texture_load_palette_blob(&palette, -1, 0) == 0);
  assert(enj_texture_load_palette_blob(&palette, PVR_PAL_ARGB8888, 1024) ==
         0);
  assert(enj_texture_load_palette_blob(&palette, PVR_PAL_ARGB8888, 7) == 1);
  assert(palette_format == PVR_PAL_ARGB8888);
  assert(palette_index == 7 && palette_value == palette.value);
  return 0;
}
