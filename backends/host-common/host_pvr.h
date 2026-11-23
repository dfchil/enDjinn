#ifndef ENJ_HOST_PVR_H
#define ENJ_HOST_PVR_H

#include <kos.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace enj_host_pvr {

struct QueuedPrimitive {
  float x[4];
  float y[4];
  float z[4];
  float u[4];
  float v[4];
  uint32_t color[4];
  uint32_t offset[4]; /* the vertices' offset colour (oargb), added when texture_mode has bit 6 */
  uint32_t count;
  uint32_t argb;
  pvr_list_t list;
  pvr_cull_mode_t culling;
  bool depth_test;
  bool depth_write;
  pvr_depthcmp_mode_t depth_cmp;
  bool alpha_cutout;
  bool textured;
  pvr_ptr_t texture;
  uint32_t texture_format;
  uint32_t texture_width;
  uint32_t texture_height;
  pvr_filter_mode_t texture_filter;
  /* PVR TSP texture fields: environment (pvr_txr_shading_mode_t) in bits 0-1, UV clamp in 2-3,
   * UV flip in 4-5 (each U = 2, V = 1, as PVR_UVCLAMP_* / PVR_UVFLIP_*), offset colour on
   * (specular, textured polygons only, as on the PVR) in bit 6. */
  uint32_t texture_mode;
  bool modifier_receiver;
  bool modifier;
  bool modifier_volume;
  bool modifier_volume_last;
  uint32_t modifier_mode;
};

constexpr uint32_t texture_env(uint32_t mode) { return mode & 3u; }
constexpr uint32_t texture_clamp(uint32_t mode) { return (mode >> 2u) & 3u; }
constexpr uint32_t texture_flip(uint32_t mode) { return (mode >> 4u) & 3u; }
constexpr uint32_t texture_offset_bit = 64u;

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

}  // namespace enj_host_pvr

#endif
