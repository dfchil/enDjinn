#include "pc_endjinn_pvr.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace {

struct TextureUpload {
  size_t capacity{};
  std::vector<uint8_t> bytes;
  uint32_t width{};
  uint32_t height{};
  uint32_t load_flags{};
  uint64_t revision{};
  bool linear{};
};

struct HeaderState {
  uint32_t argb{0xffffffffu};
  bool textured{};
  pvr_ptr_t texture{};
  uint32_t format{};
  uint32_t width{};
  uint32_t height{};
  pvr_filter_mode_t filter{PVR_FILTER_NEAREST};
  bool sprite{};
  bool modifier{};
  bool modifier_volume{};
  uint32_t modifier_mode{};
  bool model1_painter{};
  bool modifier_textured{};
  pvr_context_txr_t modifier_texture{};
  pvr_cull_mode_t culling{PVR_CULLING_NONE};
  bool alpha_cutout{};
};

alignas(32) std::array<uint8_t, 96> g_dr_packet{};
uint32_t g_dr_slot = 0u;
pvr_list_t g_current_list = PVR_LIST_OP_POLY;
pvr_sprite_txr_t g_sprite_first{};
bool g_has_sprite_first = false;
pvr_vertex_tpcm_t g_tpcm_first{};
bool g_has_tpcm_first = false;
pvr_modifier_vol_t g_modifier_volume_first{};
bool g_has_modifier_volume_first = false;
std::vector<pvr_vertex_t> g_triangle_vertices;
std::vector<pc_endjinn_pvr::QueuedPrimitive> g_primitives;
HeaderState g_header;
std::unordered_map<pvr_ptr_t, TextureUpload> g_uploads;
std::array<uint32_t, 1024> g_palette{};
pvr_palfmt_t g_palette_format = PVR_PAL_ARGB8888;
uint64_t g_palette_revision = 1u;
uint64_t g_palette_format_revision = 1u;
std::array<uint64_t, 64> g_palette_4bpp_revisions{};
std::array<uint64_t, 4> g_palette_8bpp_revisions{};
uint64_t g_texture_revision = 1u;
uintptr_t g_next_texture_handle = 0x10000u;
uint32_t g_next_modifier_texture_id = 1u;
std::unordered_map<uint32_t, pvr_context_txr_t> g_modifier_textures;

float unpack_uv(uint32_t bits) {
  float value;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

void unpack_uv_pair(uint32_t packed, float &u, float &v) {
  u = unpack_uv(packed & 0xffff0000u);
  v = unpack_uv(packed << 16u);
}

void copy_header_state(pc_endjinn_pvr::QueuedPrimitive &primitive) {
  primitive.argb = g_header.argb;
  primitive.list = g_current_list;
  primitive.culling = g_header.culling;
  primitive.alpha_cutout = g_header.alpha_cutout;
  primitive.model1_painter = g_header.model1_painter;
  primitive.textured = g_header.textured;
  primitive.texture = g_header.texture;
  primitive.texture_format = g_header.format;
  primitive.texture_width = g_header.width;
  primitive.texture_height = g_header.height;
  primitive.texture_filter = g_header.filter;
  primitive.modifier = g_header.modifier;
  primitive.modifier_volume = false;
  primitive.modifier_mode = 0u;
}

void queue_triangle(const pvr_vertex_t &a, const pvr_vertex_t &b,
                    const pvr_vertex_t &c) {
  pc_endjinn_pvr::QueuedPrimitive primitive{};
  primitive.count = 3u;
  copy_header_state(primitive);
  primitive.argb = c.argb != 0u ? c.argb : primitive.argb;
  const pvr_vertex_t vertices[3] = {a, b, c};
  for (uint32_t i = 0; i < 3u; i++) {
    primitive.x[i] = vertices[i].x;
    primitive.y[i] = vertices[i].y;
    primitive.z[i] = vertices[i].z;
    primitive.u[i] = vertices[i].u;
    primitive.v[i] = vertices[i].v;
    primitive.color[i] = vertices[i].argb != 0u ? vertices[i].argb
                                                 : primitive.argb;
  }
  g_primitives.push_back(primitive);
}

void queue_modifier_triangle(const pvr_vertex_t outside[3],
                             const pvr_vertex_t inside[3]) {
  pc_endjinn_pvr::QueuedPrimitive primitive{};
  primitive.count = 3u;
  copy_header_state(primitive);
  primitive.modifier = false;
  for (uint32_t i = 0; i < 3u; i++) {
    primitive.x[i] = outside[i].x;
    primitive.y[i] = outside[i].y;
    primitive.z[i] = outside[i].z;
    primitive.u[i] = outside[i].u;
    primitive.v[i] = outside[i].v;
    primitive.color[i] = outside[i].argb;
  }
  g_primitives.push_back(primitive);

  copy_header_state(primitive);
  primitive.modifier = true;
  if (g_header.modifier_textured) {
    primitive.textured = true;
    primitive.texture = g_header.modifier_texture.base;
    primitive.texture_format = g_header.modifier_texture.format;
    primitive.texture_width = g_header.modifier_texture.width;
    primitive.texture_height = g_header.modifier_texture.height;
    primitive.texture_filter = g_header.modifier_texture.filter;
  } else {
    primitive.textured = false;
  }
  for (uint32_t i = 0; i < 3u; i++) {
    primitive.x[i] = inside[i].x;
    primitive.y[i] = inside[i].y;
    primitive.z[i] = inside[i].z;
    primitive.u[i] = inside[i].u;
    primitive.v[i] = inside[i].v;
    primitive.color[i] = inside[i].argb;
  }
  g_primitives.push_back(primitive);
}

void queue_modifier_triangle(const pvr_vertex_pcm_t &a,
                             const pvr_vertex_pcm_t &b,
                             const pvr_vertex_pcm_t &c) {
  pvr_vertex_t outside[3] = {}, inside[3] = {};
  const pvr_vertex_pcm_t source[3] = {a, b, c};
  for (uint32_t i = 0; i < 3u; i++) {
    outside[i].x = inside[i].x = source[i].x;
    outside[i].y = inside[i].y = source[i].y;
    outside[i].z = inside[i].z = source[i].z;
    outside[i].argb = source[i].argb0;
    inside[i].argb = source[i].argb1;
  }
  queue_modifier_triangle(outside, inside);
}

void queue_modifier_triangle(const pvr_vertex_tpcm_t &a,
                             const pvr_vertex_tpcm_t &b,
                             const pvr_vertex_tpcm_t &c) {
  pvr_vertex_t outside[3] = {}, inside[3] = {};
  const pvr_vertex_tpcm_t source[3] = {a, b, c};
  for (uint32_t i = 0; i < 3u; i++) {
    outside[i].x = inside[i].x = source[i].x;
    outside[i].y = inside[i].y = source[i].y;
    outside[i].z = inside[i].z = source[i].z;
    outside[i].u = source[i].u0;
    outside[i].v = source[i].v0;
    outside[i].argb = source[i].argb0;
    inside[i].u = source[i].u1;
    inside[i].v = source[i].v1;
    inside[i].argb = source[i].argb1;
  }
  queue_modifier_triangle(outside, inside);
}

void queue_modifier_volume(const pvr_modifier_vol_t &first, const void *tail) {
  const float *values = static_cast<const float *>(tail);
  pc_endjinn_pvr::QueuedPrimitive primitive{};
  primitive.count = 3u;
  primitive.list = g_current_list;
  primitive.culling = g_header.culling;
  primitive.modifier_volume = true;
  primitive.modifier_mode = g_header.modifier_mode;
  primitive.x[0] = first.ax;
  primitive.y[0] = first.ay;
  primitive.z[0] = first.az;
  primitive.x[1] = first.bx;
  primitive.y[1] = first.by;
  primitive.z[1] = first.bz;
  primitive.x[2] = first.cx;
  primitive.y[2] = values[0];
  primitive.z[2] = values[1];
  g_primitives.push_back(primitive);
}

void queue_sprite_second_half(const void *ptr) {
  const float *tail = static_cast<const float *>(ptr);
  const uint32_t *tail_words = static_cast<const uint32_t *>(ptr);
  pc_endjinn_pvr::QueuedPrimitive primitive{};
  primitive.count = 4u;
  copy_header_state(primitive);
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
  primitive.z[3] = !g_header.textured && tail_words[5] == 0x50435a44u
      ? tail[4]
      : g_sprite_first.bz + tail[1] - g_sprite_first.az;
  if (g_header.textured) {
    unpack_uv_pair(tail_words[5], primitive.u[0], primitive.v[0]);
    unpack_uv_pair(tail_words[6], primitive.u[1], primitive.v[1]);
    unpack_uv_pair(tail_words[7], primitive.u[2], primitive.v[2]);
    primitive.u[3] = primitive.u[0] + primitive.u[2] - primitive.u[1];
    primitive.v[3] = primitive.v[0] + primitive.v[2] - primitive.v[1];
  }
  for (uint32_t i = 0; i < primitive.count; i++) {
    primitive.color[i] = primitive.argb;
  }
  g_primitives.push_back(primitive);
  g_has_sprite_first = false;
}

uint32_t log2_size(uint32_t value) {
  uint32_t result = 0u;
  while (value > 1u) {
    value >>= 1u;
    result++;
  }
  return result;
}

size_t mip_offset(uint32_t pixel_format, bool vq, uint32_t level) {
  static constexpr size_t offsets[] = {
      0x00006u, 0x00008u, 0x00010u, 0x00030u, 0x000b0u, 0x002b0u,
      0x00ab0u, 0x02ab0u, 0x0aab0u, 0x2aab0u, 0xaaab0u};
  size_t offset = offsets[std::min<size_t>(level, 10u)];
  if (vq) {
    return offset / 8u;
  }
  if (pixel_format == PVR_PIXEL_MODE_PAL_4BPP) {
    return offset / 4u;
  }
  if (pixel_format == PVR_PIXEL_MODE_PAL_8BPP) {
    return offset / 2u;
  }
  return offset;
}

size_t level_bytes(uint32_t pixel_format, uint32_t width, uint32_t height) {
  const size_t pixels = static_cast<size_t>(width) * height;
  if (pixel_format == PVR_PIXEL_MODE_PAL_4BPP) {
    return (pixels + 1u) / 2u;
  }
  if (pixel_format == PVR_PIXEL_MODE_PAL_8BPP) {
    return pixels;
  }
  return pixels * 2u;
}

size_t twiddled_index(uint32_t x, uint32_t y, uint32_t width,
                      uint32_t height) {
  const uint32_t xb = log2_size(width);
  const uint32_t yb = log2_size(height);
  const uint32_t shared = std::min(xb, yb);
  size_t index = 0u;
  for (uint32_t bit = 0u; bit < shared; bit++) {
    index |= static_cast<size_t>((y >> bit) & 1u) << (bit * 2u);
    index |= static_cast<size_t>((x >> bit) & 1u) << (bit * 2u + 1u);
  }
  if (xb > shared) {
    index |= static_cast<size_t>(x >> shared) << (shared * 2u);
  } else if (yb > shared) {
    index |= static_cast<size_t>(y >> shared) << (shared * 2u);
  }
  return index;
}

uint8_t expand4(uint32_t value) { return static_cast<uint8_t>(value * 17u); }
uint8_t expand5(uint32_t value) {
  return static_cast<uint8_t>((value << 3u) | (value >> 2u));
}
uint8_t expand6(uint32_t value) {
  return static_cast<uint8_t>((value << 2u) | (value >> 4u));
}

uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return static_cast<uint32_t>(r) | (static_cast<uint32_t>(g) << 8u) |
         (static_cast<uint32_t>(b) << 16u) |
         (static_cast<uint32_t>(a) << 24u);
}

uint32_t decode_16bit(uint16_t pixel, uint32_t format) {
  switch (format) {
    case PVR_PIXEL_MODE_ARGB1555:
      return rgba(expand5((pixel >> 10u) & 31u),
                  expand5((pixel >> 5u) & 31u), expand5(pixel & 31u),
                  (pixel & 0x8000u) != 0u ? 255u : 0u);
    case PVR_PIXEL_MODE_RGB565:
      return rgba(expand5((pixel >> 11u) & 31u),
                  expand6((pixel >> 5u) & 63u), expand5(pixel & 31u), 255u);
    case PVR_PIXEL_MODE_ARGB4444:
      return rgba(expand4((pixel >> 8u) & 15u),
                  expand4((pixel >> 4u) & 15u), expand4(pixel & 15u),
                  expand4((pixel >> 12u) & 15u));
    default:
      return rgba(static_cast<uint8_t>(pixel),
                  static_cast<uint8_t>(pixel >> 8u), 0u, 255u);
  }
}

uint8_t clamp_byte(int value) {
  return static_cast<uint8_t>(std::min(std::max(value, 0), 255));
}

void decode_yuv_pair(uint16_t left, uint16_t right, uint32_t *out) {
  const int y0 = left >> 8u;
  const int y1 = right >> 8u;
  const int u = static_cast<int>(left & 0xffu) - 128;
  const int v = static_cast<int>(right & 0xffu) - 128;
  const auto convert = [&](int y) {
    return rgba(clamp_byte(y + ((91881 * v) >> 16)),
                clamp_byte(y - ((22554 * u + 46802 * v) >> 16)),
                clamp_byte(y + ((116130 * u) >> 16)), 255u);
  };
  out[0] = convert(y0);
  out[1] = convert(y1);
}

bool source_level(const TextureUpload &upload, uint32_t format,
                  uint32_t width, uint32_t height, uint32_t source_level,
                  std::vector<uint8_t> &bytes) {
  const uint32_t pixel_format = (format >> 27u) & 7u;
  const bool vq = (format & PVR_TXRFMT_VQ_ENABLE) != 0u && !upload.linear;
  const bool mipmapped = (format & PVR_TXRFMT_MIPMAP) != 0u && !upload.linear;
  size_t offset = mipmapped ? mip_offset(pixel_format, vq, source_level) : 0u;
  size_t size = level_bytes(pixel_format, width, height);
  if (!vq) {
    if (offset > upload.bytes.size() || size > upload.bytes.size() - offset) {
      return false;
    }
    bytes.assign(upload.bytes.begin() + static_cast<ptrdiff_t>(offset),
                 upload.bytes.begin() + static_cast<ptrdiff_t>(offset + size));
    return true;
  }

  const size_t index_count = (size + 7u) / 8u;
  const uint32_t top_level = log2_size(std::max(width, height));
  const size_t all_index_bytes = mipmapped
      ? mip_offset(pixel_format, true, top_level) + index_count
      : index_count;
  if (upload.bytes.size() < all_index_bytes) {
    return false;
  }
  const size_t codebook_size =
      std::min<size_t>(2048u, upload.bytes.size() - all_index_bytes);
  const size_t indices_base = codebook_size;
  if (offset > upload.bytes.size() - indices_base ||
      index_count > upload.bytes.size() - indices_base - offset) {
    return false;
  }
  bytes.resize(index_count * 8u);
  for (size_t i = 0u; i < index_count; i++) {
    const size_t entry = upload.bytes[indices_base + offset + i];
    const size_t codebook_offset = entry * 8u;
    if (codebook_offset + 8u > codebook_size) {
      return false;
    }
    std::memcpy(bytes.data() + i * 8u,
                upload.bytes.data() + codebook_offset, 8u);
  }
  return true;
}

bool decode_level(const std::vector<uint8_t> &source, uint32_t format,
                  uint32_t width, uint32_t height, bool linear,
                  pc_endjinn_pvr::DecodedMip &decoded) {
  const uint32_t pixel_format = (format >> 27u) & 7u;
  const bool indexed = pixel_format == PVR_PIXEL_MODE_PAL_4BPP ||
                       pixel_format == PVR_PIXEL_MODE_PAL_8BPP;
  const bool twiddled = !linear &&
      (indexed || (format & PVR_TXRFMT_NONTWIDDLED) == 0u);
  const size_t pixels = static_cast<size_t>(width) * height;
  decoded.width = width;
  decoded.height = height;
  decoded.pixels.resize(pixels * (indexed ? 1u : 4u));

  std::vector<uint32_t> converted;
  if (!indexed) {
    converted.resize(pixels);
    if (pixel_format == PVR_PIXEL_MODE_YUV422) {
      const uint16_t *words = reinterpret_cast<const uint16_t *>(source.data());
      if (twiddled) {
        for (size_t i = 0u; i + 3u < pixels; i += 4u) {
          uint32_t first[2], second[2];
          decode_yuv_pair(words[i], words[i + 2u], first);
          decode_yuv_pair(words[i + 1u], words[i + 3u], second);
          converted[i] = first[0];
          converted[i + 1u] = second[0];
          converted[i + 2u] = first[1];
          converted[i + 3u] = second[1];
        }
      } else {
        for (size_t i = 0u; i + 1u < pixels; i += 2u) {
          decode_yuv_pair(words[i], words[i + 1u], &converted[i]);
        }
      }
    } else {
      for (size_t i = 0u; i < pixels; i++) {
        uint16_t word;
        std::memcpy(&word, source.data() + i * 2u, sizeof(word));
        converted[i] = decode_16bit(word, pixel_format);
      }
    }
  }

  for (uint32_t y = 0u; y < height; y++) {
    for (uint32_t x = 0u; x < width; x++) {
      const size_t dst = static_cast<size_t>(y) * width + x;
      const size_t src = twiddled ? twiddled_index(x, y, width, height) : dst;
      if (indexed) {
        decoded.pixels[dst] = pixel_format == PVR_PIXEL_MODE_PAL_4BPP
            ? static_cast<uint8_t>((source[src >> 1u] >> ((src & 1u) * 4u)) & 15u)
            : source[src];
      } else {
        std::memcpy(decoded.pixels.data() + dst * 4u, &converted[src], 4u);
      }
    }
  }
  return true;
}

}  // namespace

namespace pc_endjinn_pvr {

const std::vector<QueuedPrimitive> &primitives() { return g_primitives; }

void scene_begin() {
  g_primitives.clear();
  g_triangle_vertices.clear();
  g_has_sprite_first = false;
  g_has_modifier_volume_first = false;
}

void list_begin(pvr_list_t list) { g_current_list = list; }

void *dr_target() {
  g_dr_slot ^= 1u;
  return g_dr_packet.data() + 32u * (g_dr_slot + 1u);
}

void dr_commit(void *ptr) {
  if (ptr == nullptr) {
    return;
  }

  if (g_has_modifier_volume_first) {
    queue_modifier_volume(g_modifier_volume_first, ptr);
    g_has_modifier_volume_first = false;
    return;
  }
  if (g_has_tpcm_first) {
    std::memcpy(reinterpret_cast<uint8_t *>(&g_tpcm_first) + 32u, ptr, 32u);
    const pvr_vertex_tpcm_t *vertex = &g_tpcm_first;
    static std::vector<pvr_vertex_tpcm_t> vertices;
    vertices.push_back(*vertex);
    if (vertices.size() >= 3u) {
      const size_t n = vertices.size();
      if ((n & 1u) == 0u) {
        queue_modifier_triangle(vertices[n - 2u], vertices[n - 3u],
                                vertices[n - 1u]);
      } else {
        queue_modifier_triangle(vertices[n - 3u], vertices[n - 2u],
                                vertices[n - 1u]);
      }
    }
    if (vertex->flags == PVR_CMD_VERTEX_EOL) {
      vertices.clear();
    }
    g_has_tpcm_first = false;
    return;
  }
  const uint32_t flags = *static_cast<const uint32_t *>(ptr);
  if (g_has_sprite_first) {
    queue_sprite_second_half(ptr);
    return;
  }

  if (g_header.sprite &&
      (flags == PVR_CMD_VERTEX || flags == PVR_CMD_VERTEX_EOL)) {
    std::memcpy(&g_sprite_first, ptr, sizeof(g_sprite_first));
    g_has_sprite_first = true;
    return;
  }

  if (flags == PVR_CMD_VERTEX || flags == PVR_CMD_VERTEX_EOL) {
    if (g_header.modifier_volume) {
        std::memcpy(&g_modifier_volume_first, ptr, 32u);
        g_has_modifier_volume_first = true;
        return;
    }
    if (g_header.modifier) {
      if (g_header.textured) {
        std::memcpy(&g_tpcm_first, ptr, 32u);
        g_has_tpcm_first = true;
        return;
      }
      const pvr_vertex_pcm_t *vertex =
          static_cast<const pvr_vertex_pcm_t *>(ptr);
      static std::vector<pvr_vertex_pcm_t> vertices;
      vertices.push_back(*vertex);
      if (vertices.size() >= 3u) {
        const size_t n = vertices.size();
        if ((n & 1u) == 0u) {
          queue_modifier_triangle(vertices[n - 2u], vertices[n - 3u],
                                  vertices[n - 1u]);
        } else {
          queue_modifier_triangle(vertices[n - 3u], vertices[n - 2u],
                                  vertices[n - 1u]);
        }
      }
      if (flags == PVR_CMD_VERTEX_EOL) {
        vertices.clear();
      }
      return;
    }
    const pvr_vertex_t *vertex = static_cast<const pvr_vertex_t *>(ptr);
    g_triangle_vertices.push_back(*vertex);
    if (g_triangle_vertices.size() >= 3u) {
      const size_t n = g_triangle_vertices.size();
      if ((n & 1u) == 0u) {
        queue_triangle(g_triangle_vertices[n - 2u],
                       g_triangle_vertices[n - 3u],
                       g_triangle_vertices[n - 1u]);
      } else {
        queue_triangle(g_triangle_vertices[n - 3u],
                       g_triangle_vertices[n - 2u],
                       g_triangle_vertices[n - 1u]);
      }
    }
    if (flags == PVR_CMD_VERTEX_EOL) {
      g_triangle_vertices.clear();
    }
    return;
  }

  const pvr_sprite_hdr_t *header = static_cast<const pvr_sprite_hdr_t *>(ptr);
  g_header.argb = header->argb != 0u ? header->argb : 0xffffffffu;
  g_header.textured = (header->mode1 & 0x80000000u) != 0u;
  g_header.format = header->mode2;
  g_header.width = header->mode3 & 0xffffu;
  g_header.height = header->mode3 >> 16u;
  g_header.filter = static_cast<pvr_filter_mode_t>(header->oargb);
  g_header.sprite = (header->mode1 & PC_ENDJINN_PVR_HEADER_SPRITE) != 0u;
  g_header.alpha_cutout =
      (header->mode1 & PC_ENDJINN_PVR_HEADER_ALPHA_CUTOUT) != 0u;
  g_header.model1_painter =
      (header->mode1 & PC_ENDJINN_PVR_HEADER_MODEL1_PAINTER) != 0u;
  g_header.modifier = (header->mode1 & 0x40000000u) != 0u;
  g_header.modifier_volume = (header->mode1 & 0x20000000u) != 0u;
  g_header.culling = static_cast<pvr_cull_mode_t>(
      (header->mode1 & PC_ENDJINN_PVR_HEADER_CULL_MASK) >>
      PC_ENDJINN_PVR_HEADER_CULL_SHIFT);
  g_header.modifier_mode = g_header.modifier_volume ? header->oargb : 0u;
  const auto modifier_texture = g_modifier_textures.find(header->cmd & 0x0fffffffu);
  g_header.modifier_textured = modifier_texture != g_modifier_textures.end();
  if (g_header.modifier_textured) {
    g_header.modifier_texture = modifier_texture->second;
  }
  uintptr_t base = header->reserved[0];
#if UINTPTR_MAX > UINT32_MAX
  base |= static_cast<uintptr_t>(header->reserved[1]) << 32u;
#endif
  g_header.texture = reinterpret_cast<pvr_ptr_t>(base);
}

void *texture_alloc(size_t size) {
  if (size == 0u || size > UINT32_MAX) {
    return nullptr;
  }
  const uintptr_t aligned_size = (size + 31u) & ~uintptr_t{31u};
  if (g_next_texture_handle > UINT32_MAX - aligned_size) {
    return nullptr;
  }
  const uintptr_t handle = g_next_texture_handle;
  g_next_texture_handle += aligned_size;
  void *ptr = reinterpret_cast<void *>(handle);
  g_uploads[ptr].capacity = size;
  return ptr;
}

void texture_free(pvr_ptr_t ptr) {
  g_uploads.erase(ptr);
}

void texture_load(const void *src, pvr_ptr_t dst, size_t count) {
  auto found = g_uploads.find(dst);
  if (src == nullptr || found == g_uploads.end() || count > found->second.capacity) {
    return;
  }
  found->second.bytes.assign(static_cast<const uint8_t *>(src),
                             static_cast<const uint8_t *>(src) + count);
  found->second.linear = false;
  found->second.revision = ++g_texture_revision;
}

void texture_load_ex(const void *src, pvr_ptr_t dst, uint32_t width,
                     uint32_t height, uint32_t flags) {
  auto found = g_uploads.find(dst);
  if (src == nullptr || found == g_uploads.end()) {
    return;
  }
  const size_t pixels = static_cast<size_t>(width) * height;
  size_t count = pixels * 2u;
  if (flags == PVR_TXRLOAD_4BPP) {
    count = (pixels + 1u) / 2u;
  } else if (flags == PVR_TXRLOAD_8BPP) {
    count = pixels;
  }
  if (count > found->second.capacity) {
    return;
  }
  found->second.bytes.assign(static_cast<const uint8_t *>(src),
                             static_cast<const uint8_t *>(src) + count);
  found->second.width = width;
  found->second.height = height;
  found->second.load_flags = flags;
  found->second.linear = true;
  found->second.revision = ++g_texture_revision;
}

void palette_format(pvr_palfmt_t format) {
  if (g_palette_format != format) {
    g_palette_format = format;
    g_palette_format_revision = ++g_palette_revision;
  }
}

void palette_entry(uint32_t index, uint32_t value) {
  if (index < g_palette.size() && g_palette[index] != value) {
    g_palette[index] = value;
    const uint64_t revision = ++g_palette_revision;
    g_palette_4bpp_revisions[index / 16u] = revision;
    g_palette_8bpp_revisions[index / 256u] = revision;
  }
}

uint64_t palette_revision() { return g_palette_revision; }

uint64_t palette_revision(uint32_t texture_format) {
  const uint32_t pixel_format = (texture_format >> 27u) & 7u;
  if (pixel_format == PVR_PIXEL_MODE_PAL_4BPP) {
    const size_t bank = (texture_format >> 21u) & 0x3fu;
    return std::max(g_palette_format_revision,
                    g_palette_4bpp_revisions[bank]);
  }
  if (pixel_format == PVR_PIXEL_MODE_PAL_8BPP) {
    const size_t bank = (texture_format >> 25u) & 0x03u;
    return std::max(g_palette_format_revision,
                    g_palette_8bpp_revisions[bank]);
  }
  return 0u;
}

uint64_t texture_revision(pvr_ptr_t ptr) {
  const auto found = g_uploads.find(ptr);
  return found == g_uploads.end() ? 0u : found->second.revision;
}

std::array<uint32_t, 1024> palette_rgba() {
  std::array<uint32_t, 1024> result{};
  for (size_t i = 0u; i < result.size(); i++) {
    const uint32_t value = g_palette[i];
    switch (g_palette_format) {
      case PVR_PAL_ARGB1555:
        result[i] = decode_16bit(static_cast<uint16_t>(value),
                                 PVR_PIXEL_MODE_ARGB1555);
        break;
      case PVR_PAL_RGB565:
        result[i] = decode_16bit(static_cast<uint16_t>(value),
                                 PVR_PIXEL_MODE_RGB565);
        break;
      case PVR_PAL_ARGB4444:
        result[i] = decode_16bit(static_cast<uint16_t>(value),
                                 PVR_PIXEL_MODE_ARGB4444);
        break;
      case PVR_PAL_ARGB8888:
        result[i] = rgba(static_cast<uint8_t>(value >> 16u),
                         static_cast<uint8_t>(value >> 8u),
                         static_cast<uint8_t>(value),
                         static_cast<uint8_t>(value >> 24u));
        break;
    }
  }
  return result;
}

bool decode_texture(const QueuedPrimitive &primitive, DecodedTexture &decoded) {
  const auto found = g_uploads.find(primitive.texture);
  if (!primitive.textured || found == g_uploads.end() ||
      primitive.texture_width == 0u || primitive.texture_height == 0u) {
    std::printf(
        "pc-enDjinn: decode_texture failed - textured=%u upload_found=%u "
        "texture=%p fmt=0x%08x width=%u height=%u\n",
        static_cast<unsigned>(primitive.textured),
        static_cast<unsigned>(found != g_uploads.end()),
        reinterpret_cast<const void *>(primitive.texture),
        primitive.texture_format, primitive.texture_width,
        primitive.texture_height);
    return false;
  }
  const TextureUpload &upload = found->second;
  const uint32_t pixel_format = (primitive.texture_format >> 27u) & 7u;
  decoded = {};
  decoded.indexed = pixel_format == PVR_PIXEL_MODE_PAL_4BPP ||
                    pixel_format == PVR_PIXEL_MODE_PAL_8BPP;
  decoded.bump = pixel_format == PVR_PIXEL_MODE_BUMP;
  decoded.palette_base = pixel_format == PVR_PIXEL_MODE_PAL_4BPP
      ? ((primitive.texture_format >> 21u) & 0x3fu) * 16u
      : ((primitive.texture_format >> 25u) & 0x03u) * 256u;

  const bool mipmapped = (primitive.texture_format & PVR_TXRFMT_MIPMAP) != 0u &&
                         !upload.linear;
  const uint32_t mip_count = mipmapped
      ? log2_size(std::max(primitive.texture_width,
                           primitive.texture_height)) + 1u
      : 1u;
  for (uint32_t mip = 0u; mip < mip_count; mip++) {
    const uint32_t width = std::max(1u, primitive.texture_width >> mip);
    const uint32_t height = std::max(1u, primitive.texture_height >> mip);
    const uint32_t source_mip = mipmapped ? log2_size(std::max(width, height)) : 0u;
    std::vector<uint8_t> source;
    if (!source_level(upload, primitive.texture_format, width, height,
                      source_mip, source)) {
      std::printf(
          "pc-enDjinn: decode_texture source_level failed - texture=%p "
          "fmt=0x%08x width=%u height=%u mip=%u bytes=%zu upload_bytes=%zu "
          "linear=%u\n",
          reinterpret_cast<const void *>(primitive.texture),
          primitive.texture_format, width, height, source_mip, source.size(),
          upload.bytes.size(), static_cast<unsigned>(upload.linear));
      return false;
    }
    DecodedMip level;
    if (!decode_level(source, primitive.texture_format, width, height,
                      upload.linear, level)) {
      std::printf(
          "pc-enDjinn: decode_texture decode_level failed - texture=%p "
          "fmt=0x%08x width=%u height=%u mip=%u linear=%u\n",
          reinterpret_cast<const void *>(primitive.texture),
          primitive.texture_format, width, height, source_mip,
          static_cast<unsigned>(upload.linear));
      return false;
    }
    decoded.mips.push_back(std::move(level));
  }
  return !decoded.mips.empty();
}

}  // namespace pc_endjinn_pvr

extern "C" uint32_t pc_endjinn_pvr_register_modifier_texture(
    const pvr_context_txr_t *texture) {
  if (texture == nullptr || !texture->enable ||
      g_next_modifier_texture_id == 0x10000000u) {
    return 0u;
  }
  for (const auto &[id, current] : g_modifier_textures) {
    if (current.enable == texture->enable && current.format == texture->format &&
        current.width == texture->width && current.height == texture->height &&
        current.base == texture->base && current.filter == texture->filter &&
        current.alpha == texture->alpha && current.env == texture->env &&
        current.uv_clamp == texture->uv_clamp) {
      return id;
    }
  }
  const uint32_t id = g_next_modifier_texture_id++;
  g_modifier_textures[id] = *texture;
  return id;
}
