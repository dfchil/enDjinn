#include "web_endjinn_frame.h"

#include "../host-common/host_pvr.h"
#include "../host-common/host_translucent_sort.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace web_endjinn {
namespace {

using enj_host_pvr::QueuedPrimitive;

FrameDrawData g_frame;

WebVertex make_vertex(const vid_mode_t &video_mode, bool fsaa_enabled,
                      float x, float y, float z, uint32_t argb, float u,
                      float v)
{
  const float half_width = video_mode.width * (fsaa_enabled ? 1.0f : 0.5f);
  const float half_height = video_mode.height * 0.5f;
  return {{x / half_width - 1.0f, 1.0f - y / half_height,
           std::clamp(z * 0.25f, 0.0f, 1.0f)},
          {static_cast<uint8_t>((argb >> 16u) & 0xffu),
           static_cast<uint8_t>((argb >> 8u) & 0xffu),
           static_cast<uint8_t>(argb & 0xffu),
           static_cast<uint8_t>((argb >> 24u) & 0xffu)},
          {u, v}};
}

bool triangle_is_culled(const QueuedPrimitive &primitive, uint32_t a,
                        uint32_t b, uint32_t c)
{
  if (primitive.culling != PVR_CULLING_CCW &&
      primitive.culling != PVR_CULLING_CW) {
    return false;
  }
  const float area =
      (primitive.x[b] - primitive.x[a]) *
          (primitive.y[c] - primitive.y[a]) -
      (primitive.y[b] - primitive.y[a]) *
          (primitive.x[c] - primitive.x[a]);
  return primitive.culling == PVR_CULLING_CCW ? area < 0.0f : area > 0.0f;
}

void emit_primitive(std::vector<WebVertex> &vertices,
                    const QueuedPrimitive &primitive,
                    const vid_mode_t &video_mode, bool fsaa_enabled)
{
  const auto emit = [&](uint32_t index) {
    vertices.push_back(make_vertex(
        video_mode, fsaa_enabled, primitive.x[index], primitive.y[index],
        primitive.z[index], primitive.color[index], primitive.u[index],
        primitive.v[index]));
  };
  if (primitive.count == 3u) {
    if (!triangle_is_culled(primitive, 0u, 1u, 2u)) {
      emit(0u);
      emit(1u);
      emit(2u);
    }
  } else if (primitive.count == 4u) {
    if (!triangle_is_culled(primitive, 0u, 1u, 2u)) {
      emit(0u);
      emit(1u);
      emit(2u);
    }
    if (!triangle_is_culled(primitive, 0u, 2u, 3u)) {
      emit(0u);
      emit(2u);
      emit(3u);
    }
  }
}

}  // namespace

const FrameDrawData &build_frame(const vid_mode_t &video_mode,
                                 bool fsaa_enabled,
                                 bool translucent_autosort)
{
  const auto &queued = enj_host_pvr::primitives();
  g_frame.vertices.clear();
  g_frame.batches.clear();
  g_frame.translucent_modifiers.clear();
  g_frame.vertices.reserve(queued.size() * 6u);
  g_frame.translucent_modifiers.reserve(queued.size());

  for (const QueuedPrimitive &primitive : queued) {
    if (primitive.list != PVR_LIST_TR_MOD || !primitive.modifier_volume ||
        primitive.count != 3u ||
        triangle_is_culled(primitive, 0u, 1u, 2u)) {
      continue;
    }
    const WebVertex a = make_vertex(video_mode, fsaa_enabled, primitive.x[0],
                                    primitive.y[0], primitive.z[0], 0u, 0.0f,
                                    0.0f);
    const WebVertex b = make_vertex(video_mode, fsaa_enabled, primitive.x[1],
                                    primitive.y[1], primitive.z[1], 0u, 0.0f,
                                    0.0f);
    const WebVertex c = make_vertex(video_mode, fsaa_enabled, primitive.x[2],
                                    primitive.y[2], primitive.z[2], 0u, 0.0f,
                                    0.0f);
    WebModifierTriangle event{};
    for (size_t component = 0u; component < 3u; component++) {
      event.a[component] = a.position[component];
      event.b[component] = b.position[component];
      event.c[component] = c.position[component];
    }
    event.state[0] = primitive.modifier_volume_last ? 1.0f : 0.0f;
    event.state[1] = !primitive.modifier_volume_last &&
                             primitive.modifier_mode != PVR_MODIFIER_OTHER_POLY
                         ? 1.0f
                         : 0.0f;
    event.state[2] = static_cast<float>(primitive.modifier_mode);
    g_frame.translucent_modifiers.push_back(event);
  }

  const auto append_list = [&](pvr_list_t list, bool sort_back_to_front,
                               bool modifier_volume, int modifier_filter) {
    std::vector<const QueuedPrimitive *> primitives;
    primitives.reserve(queued.size());
    for (const QueuedPrimitive &primitive : queued) {
      if (primitive.list == list &&
          primitive.modifier_volume == modifier_volume &&
          (modifier_filter < 0 ||
           primitive.modifier == (modifier_filter != 0))) {
        primitives.push_back(&primitive);
      }
    }
    if (sort_back_to_front) {
      (void)enj_host_translucent_sort::sort(primitives);
    }
    for (const QueuedPrimitive *primitive : primitives) {
      const bool same = !g_frame.batches.empty() &&
                        g_frame.batches.back().list == list &&
                        g_frame.batches.back().depth_test ==
                            primitive->depth_test &&
                        g_frame.batches.back().depth_write ==
                            primitive->depth_write &&
                        g_frame.batches.back().alpha_cutout ==
                            primitive->alpha_cutout &&
                        g_frame.batches.back().textured == primitive->textured &&
                        g_frame.batches.back().texture == primitive->texture &&
                        g_frame.batches.back().texture_format ==
                            primitive->texture_format &&
                        g_frame.batches.back().texture_width ==
                            primitive->texture_width &&
                        g_frame.batches.back().texture_height ==
                            primitive->texture_height &&
                        g_frame.batches.back().texture_filter ==
                            primitive->texture_filter &&
                        g_frame.batches.back().modifier_receiver ==
                            primitive->modifier_receiver &&
                        g_frame.batches.back().modifier == primitive->modifier &&
                        g_frame.batches.back().modifier_volume ==
                            primitive->modifier_volume &&
                        g_frame.batches.back().modifier_volume_last ==
                            primitive->modifier_volume_last &&
                        g_frame.batches.back().modifier_mode ==
                            primitive->modifier_mode;
      if (!same) {
        DrawBatch batch{};
        batch.list = list;
        batch.first_vertex = static_cast<uint32_t>(g_frame.vertices.size());
        batch.depth_test = primitive->depth_test;
        batch.depth_write = primitive->depth_write;
        batch.alpha_cutout = primitive->alpha_cutout;
        batch.textured = primitive->textured;
        batch.texture = primitive->texture;
        batch.texture_format = primitive->texture_format;
        batch.texture_width = primitive->texture_width;
        batch.texture_height = primitive->texture_height;
        batch.texture_filter = primitive->texture_filter;
        batch.modifier_receiver = primitive->modifier_receiver;
        batch.modifier = primitive->modifier;
        batch.modifier_volume = primitive->modifier_volume;
        batch.modifier_volume_last = primitive->modifier_volume_last;
        batch.modifier_mode = primitive->modifier_mode;
        g_frame.batches.push_back(batch);
      }
      const uint32_t before = static_cast<uint32_t>(g_frame.vertices.size());
      emit_primitive(g_frame.vertices, *primitive, video_mode, fsaa_enabled);
      g_frame.batches.back().vertex_count +=
          static_cast<uint32_t>(g_frame.vertices.size()) - before;
    }
  };

  /* Area 0 establishes receiver depth before volume classification. */
  append_list(PVR_LIST_OP_POLY, false, false, 0);
  append_list(PVR_LIST_OP_MOD, false, true, -1);
  append_list(PVR_LIST_OP_POLY, false, false, 1);
  append_list(PVR_LIST_PT_POLY, false, false, -1);
  append_list(PVR_LIST_TR_POLY, translucent_autosort, false, -1);
  return g_frame;
}

}  // namespace web_endjinn
