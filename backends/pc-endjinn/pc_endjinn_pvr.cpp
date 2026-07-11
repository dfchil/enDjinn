#include "pc_endjinn_pvr.h"

#include <array>
#include <cstring>
#include <vector>

namespace {

std::array<uint8_t, 256> g_dr_packet{};
uint32_t g_current_argb = 0xffffffffu;
pvr_list_t g_current_list = PVR_LIST_OP_POLY;
pvr_sprite_col_t g_sprite_first{};
bool g_has_sprite_first = false;
std::vector<pvr_vertex_t> g_triangle_vertices;
std::vector<pc_endjinn_pvr::QueuedPrimitive> g_primitives;

void queue_triangle(const pvr_vertex_t &a, const pvr_vertex_t &b,
                    const pvr_vertex_t &c) {
  pc_endjinn_pvr::QueuedPrimitive primitive{};
  primitive.count = 3u;
  primitive.argb = c.argb != 0u ? c.argb : g_current_argb;
  primitive.list = g_current_list;
  primitive.x[0] = a.x;
  primitive.y[0] = a.y;
  primitive.z[0] = a.z;
  primitive.x[1] = b.x;
  primitive.y[1] = b.y;
  primitive.z[1] = b.z;
  primitive.x[2] = c.x;
  primitive.y[2] = c.y;
  primitive.z[2] = c.z;
  g_primitives.push_back(primitive);
}

void queue_sprite_second_half(const void *ptr) {
  const float *tail = static_cast<const float *>(ptr);
  pc_endjinn_pvr::QueuedPrimitive primitive{};
  primitive.count = 4u;
  primitive.argb = g_current_argb;
  primitive.list = g_current_list;
  primitive.x[0] = g_sprite_first.ax;
  primitive.y[0] = g_sprite_first.ay;
  primitive.z[0] = g_sprite_first.az;
  primitive.x[1] = g_sprite_first.bx;
  primitive.y[1] = g_sprite_first.by;
  primitive.z[1] = g_sprite_first.bz;
  primitive.x[2] = g_sprite_first.cx;
  primitive.y[2] = tail[0];
  primitive.z[2] = tail[1];
  primitive.x[3] = tail[2];
  primitive.y[3] = tail[3];
  const uint32_t *tail_words = static_cast<const uint32_t *>(ptr);
  primitive.z[3] = tail_words[5] == 0x50435a44u
      ? tail[4]
      : g_sprite_first.bz + tail[1] - g_sprite_first.az;
  g_primitives.push_back(primitive);
  g_has_sprite_first = false;
}

}  // namespace

namespace pc_endjinn_pvr {

const std::vector<QueuedPrimitive> &primitives() { return g_primitives; }

void scene_begin() {
  g_primitives.clear();
  g_triangle_vertices.clear();
  g_has_sprite_first = false;
}

void list_begin(pvr_list_t list) { g_current_list = list; }

void *dr_target() {
  if (!g_has_sprite_first) {
    std::memset(g_dr_packet.data(), 0, g_dr_packet.size());
    return g_dr_packet.data();
  }
  std::memset(g_dr_packet.data() + 32u, 0, g_dr_packet.size() - 32u);
  return g_dr_packet.data() + 32u;
}

void dr_commit(void *ptr) {
  if (ptr == nullptr) {
    return;
  }

  const uint32_t flags = *static_cast<const uint32_t *>(ptr);
  if (g_has_sprite_first) {
    queue_sprite_second_half(ptr);
    return;
  }

  if (flags == PVR_CMD_VERTEX || flags == PVR_CMD_VERTEX_EOL) {
    if (flags == PVR_CMD_VERTEX_EOL && g_triangle_vertices.empty()) {
      std::memcpy(&g_sprite_first, ptr, sizeof(g_sprite_first));
      g_has_sprite_first = true;
      return;
    }

    const pvr_vertex_t *vertex = static_cast<const pvr_vertex_t *>(ptr);
    g_triangle_vertices.push_back(*vertex);
    if (flags == PVR_CMD_VERTEX_EOL) {
      if (g_triangle_vertices.size() >= 3u) {
        const size_t n = g_triangle_vertices.size();
        queue_triangle(g_triangle_vertices[n - 3u],
                       g_triangle_vertices[n - 2u],
                       g_triangle_vertices[n - 1u]);
      }
      g_triangle_vertices.clear();
    }
    return;
  }

  const pvr_sprite_hdr_t *header = static_cast<const pvr_sprite_hdr_t *>(ptr);
  if (header->argb != 0u) {
    g_current_argb = header->argb;
  }
}

}  // namespace pc_endjinn_pvr
