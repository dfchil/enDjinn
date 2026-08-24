#ifndef PC_ENDJINN_PVR_H
#define PC_ENDJINN_PVR_H

#include <kos.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pc_endjinn_pvr {

struct QueuedPrimitive {
  float x[4];
  float y[4];
  float z[4];
  float u[4];
  float v[4];
  uint32_t color[4];
  uint32_t count;
  uint32_t argb;
  pvr_list_t list;
  pvr_cull_mode_t culling;
  bool depth_write;
  bool alpha_cutout;
  bool model1_painter;
  bool textured;
  pvr_ptr_t texture;
  uint32_t texture_format;
  uint32_t texture_width;
  uint32_t texture_height;
  pvr_filter_mode_t texture_filter;
  bool modifier;
  bool modifier_volume;
  uint32_t modifier_mode;
};

struct DecodedMip {
  uint32_t width;
  uint32_t height;
  std::vector<uint8_t> pixels;
};

struct DecodedTexture {
  bool indexed;
  bool bump;
  uint32_t palette_base;
  std::vector<DecodedMip> mips;
};

const std::vector<QueuedPrimitive> &primitives();
void scene_begin();
void list_begin(pvr_list_t list);
void *dr_target();
void dr_commit(void *ptr);
void *texture_alloc(size_t size);
void texture_free(pvr_ptr_t ptr);
void texture_load(const void *src, pvr_ptr_t dst, size_t count);
void texture_load_ex(const void *src, pvr_ptr_t dst, uint32_t width,
                     uint32_t height, uint32_t flags);
void palette_format(pvr_palfmt_t format);
void palette_entry(uint32_t index, uint32_t value);
uint64_t palette_revision();
uint64_t palette_revision(uint32_t texture_format);
uint64_t texture_revision(pvr_ptr_t ptr);
std::array<uint32_t, 1024> palette_rgba();
bool decode_texture(const QueuedPrimitive &primitive, DecodedTexture &decoded);

}  // namespace pc_endjinn_pvr

#endif
