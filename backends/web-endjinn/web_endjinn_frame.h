#ifndef WEB_ENDJINN_FRAME_H
#define WEB_ENDJINN_FRAME_H

#include <kos.h>

#include <array>
#include <cstdint>
#include <vector>

namespace web_endjinn {

struct WebVertex {
  float position[3];
  uint8_t color[4];
  float uv[2];
};

static_assert(sizeof(WebVertex) == 24u);

struct DrawBatch {
  pvr_list_t list{};
  uint32_t first_vertex{};
  uint32_t vertex_count{};
  bool depth_test{};
  bool depth_write{};
  bool alpha_cutout{};
  bool textured{};
  pvr_ptr_t texture{};
  uint32_t texture_format{};
  uint32_t texture_width{};
  uint32_t texture_height{};
  pvr_filter_mode_t texture_filter{};
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
