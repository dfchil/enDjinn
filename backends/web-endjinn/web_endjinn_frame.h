#ifndef WEB_ENDJINN_FRAME_H
#define WEB_ENDJINN_FRAME_H

#include <kos.h>

#include <array>
#include <cstdint>
#include <vector>

namespace web_endjinn {

struct WebVertex {
  /* NDC x and y, depth, and the vertex's 1/w as submitted (the PVR's z): the
   * vertex shader turns 1/w back into clip-space w, so the GPU interpolates
   * UVs and colours with perspective, as the PVR does. */
  float position[4];
  uint8_t color[4];
  uint8_t offset[4]; /* PVR offset colour, rgb (added when the batch has texture_offset_bit) */
  float uv[2];
};

static_assert(sizeof(WebVertex) == 32u);

struct DrawBatch {
  pvr_list_t list{};
  uint32_t first_vertex{};
  uint32_t vertex_count{};
  bool depth_test{};
  bool depth_write{};
  pvr_depthcmp_mode_t depth_cmp{PVR_DEPTHCMP_GREATER};
  bool alpha_cutout{};
  bool textured{};
  pvr_ptr_t texture{};
  uint32_t texture_format{};
  uint32_t texture_width{};
  uint32_t texture_height{};
  pvr_filter_mode_t texture_filter{};
  uint32_t texture_mode{};  // enj_host_pvr::QueuedPrimitive::texture_mode
  bool modifier_receiver{};
  bool modifier{};
  bool modifier_volume{};
  bool modifier_volume_last{};
  uint32_t modifier_mode{};
};

struct WebModifierTriangle {
  std::array<float, 4> a{};
  std::array<float, 4> b{};
  std::array<float, 4> c{};
  std::array<float, 4> state{};
};

struct FrameDrawData {
  std::vector<WebVertex> vertices;
  std::vector<DrawBatch> batches;
  std::vector<WebModifierTriangle> translucent_modifiers;
};

const FrameDrawData &build_frame(const vid_mode_t &video_mode,
                                 bool fsaa_enabled,
                                 bool translucent_autosort);

}  // namespace web_endjinn

#endif
