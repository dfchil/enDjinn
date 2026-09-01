#include "../backends/pc-endjinn/pc_endjinn_pvr.h"

#include <cassert>
#include <cstdint>

int main() {
  pc_endjinn_pvr::scene_begin();
  pc_endjinn_pvr::list_begin(PVR_LIST_TR_POLY);
  pvr_poly_cxt_t context{};
  pvr_poly_cxt_col(&context, PVR_LIST_TR_POLY);
  auto *header = static_cast<pvr_poly_hdr_t *>(pc_endjinn_pvr::dr_target());
  pvr_poly_compile(header, &context);
  pc_endjinn_pvr::dr_commit(header);

  auto *vertex = static_cast<pvr_vertex_t *>(pc_endjinn_pvr::dr_target());
  *vertex = {PVR_CMD_VERTEX, 10.0f, 20.0f, 0.2f, 0.0f, 0.0f,
             0xffffffffu, 0u};
  pc_endjinn_pvr::dr_commit(vertex);
  vertex = static_cast<pvr_vertex_t *>(pc_endjinn_pvr::dr_target());
  *vertex = {PVR_CMD_VERTEX, 10.0f, 28.0f, 0.2f, 0.0f, 1.0f,
             0xffffffffu, 0u};
  pc_endjinn_pvr::dr_commit(vertex);
  vertex = static_cast<pvr_vertex_t *>(pc_endjinn_pvr::dr_target());
  vertex->x = 30.0f;
  pc_endjinn_pvr::dr_commit(vertex);
  vertex = static_cast<pvr_vertex_t *>(pc_endjinn_pvr::dr_target());
  vertex->flags = PVR_CMD_VERTEX_EOL;
  vertex->x = 30.0f;
  pc_endjinn_pvr::dr_commit(vertex);

  const auto &submitted = pc_endjinn_pvr::primitives();
  assert(submitted.size() == 2u);
  assert(!submitted[0].modifier_receiver && !submitted[1].modifier_receiver);
  assert(submitted[0].x[2] == 30.0f && submitted[0].y[2] == 20.0f);
  assert(submitted[1].x[2] == 30.0f && submitted[1].y[2] == 28.0f);

  pc_endjinn_pvr::scene_begin();
  pc_endjinn_pvr::list_begin(PVR_LIST_OP_MOD);
  const auto submit_modifier = [](uint32_t mode, float z) {
    auto *modifier_header =
        static_cast<pvr_mod_hdr_t *>(pc_endjinn_pvr::dr_target());
    pvr_mod_compile(modifier_header, PVR_LIST_OP_MOD, mode,
                    PVR_CULLING_NONE);
    pc_endjinn_pvr::dr_commit(modifier_header);

    auto *first =
        static_cast<pvr_modifier_vol_t *>(pc_endjinn_pvr::dr_target());
    first->flags = PVR_CMD_VERTEX_EOL;
    first->ax = 10.0f;
    first->ay = 10.0f;
    first->az = z;
    first->bx = 20.0f;
    first->by = 10.0f;
    first->bz = z;
    first->cx = 10.0f;
    pc_endjinn_pvr::dr_commit(first);
    auto *tail = static_cast<float *>(pc_endjinn_pvr::dr_target());
    tail[0] = 20.0f;
    tail[1] = z;
    pc_endjinn_pvr::dr_commit(tail);
  };
  submit_modifier(PVR_MODIFIER_OTHER_POLY, 0.75f);
  submit_modifier(PVR_MODIFIER_INCLUDE_LAST_POLY, 0.25f);
  const auto &modifiers = pc_endjinn_pvr::primitives();
  assert(modifiers.size() == 2u);
  assert(!modifiers[0].modifier_receiver && modifiers[0].modifier_volume &&
         !modifiers[0].modifier_volume_last &&
         modifiers[0].modifier_mode == PVR_MODIFIER_OTHER_POLY);
  assert(modifiers[1].modifier_volume &&
         modifiers[1].modifier_volume_last &&
         modifiers[1].modifier_mode == PVR_MODIFIER_INCLUDE_LAST_POLY);

  pc_endjinn_pvr::scene_begin();
  pc_endjinn_pvr::list_begin(PVR_LIST_TR_POLY);
  pvr_poly_cxt_col_mod(&context, PVR_LIST_TR_POLY);
  auto *modifier_receiver_header =
      static_cast<pvr_poly_mod_hdr_t *>(pc_endjinn_pvr::dr_target());
  pvr_poly_mod_compile(modifier_receiver_header, &context);
  pc_endjinn_pvr::dr_commit(modifier_receiver_header);
  for (int i = 0; i < 3; i++) {
    auto *receiver =
        static_cast<pvr_vertex_pcm_t *>(pc_endjinn_pvr::dr_target());
    receiver->flags = i == 2 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
    receiver->x = 10.0f + static_cast<float>(i * 10);
    receiver->y = 20.0f + static_cast<float>((i & 1) * 10);
    receiver->z = 0.4f;
    receiver->argb0 = 0x80ff0000u;
    receiver->argb1 = 0x8000ff00u;
    pc_endjinn_pvr::dr_commit(receiver);
  }
  const auto &receiver_areas = pc_endjinn_pvr::primitives();
  assert(receiver_areas.size() == 2u);
  assert(receiver_areas[0].modifier_receiver && !receiver_areas[0].modifier);
  assert(receiver_areas[1].modifier_receiver && receiver_areas[1].modifier);

  /* The raw PVR combination used for an open OR volume has a non-zero
   * modifier instruction without the volume-last flag. */
  pc_endjinn_pvr::scene_begin();
  pc_endjinn_pvr::list_begin(PVR_LIST_OP_MOD);
  auto *open_header =
      static_cast<pvr_mod_hdr_t *>(pc_endjinn_pvr::dr_target());
  pvr_mod_compile(open_header, PVR_LIST_OP_MOD, PVR_MODIFIER_OTHER_POLY,
                  PVR_CULLING_NONE);
  open_header->oargb = PVR_MODIFIER_INCLUDE_LAST_POLY;
  pc_endjinn_pvr::dr_commit(open_header);
  auto *open_first =
      static_cast<pvr_modifier_vol_t *>(pc_endjinn_pvr::dr_target());
  open_first->flags = PVR_CMD_VERTEX_EOL;
  open_first->ax = 10.0f;
  open_first->ay = 10.0f;
  open_first->az = 0.75f;
  open_first->bx = 20.0f;
  open_first->by = 10.0f;
  open_first->bz = 0.75f;
  open_first->cx = 10.0f;
  pc_endjinn_pvr::dr_commit(open_first);
  auto *open_tail = static_cast<float *>(pc_endjinn_pvr::dr_target());
  open_tail[0] = 20.0f;
  open_tail[1] = 0.75f;
  pc_endjinn_pvr::dr_commit(open_tail);
  const auto &open_modifiers = pc_endjinn_pvr::primitives();
  assert(open_modifiers.size() == 1u);
  assert(open_modifiers[0].modifier_volume &&
         !open_modifiers[0].modifier_volume_last &&
         open_modifiers[0].modifier_mode ==
             PVR_MODIFIER_INCLUDE_LAST_POLY);

  const uint16_t argb1555[] = {0xffffu, 0x8000u, 0x7fffu, 0x0000u};
  pvr_ptr_t texture = pc_endjinn_pvr::texture_alloc(sizeof(argb1555));
  assert(reinterpret_cast<uintptr_t>(texture) <= UINT32_MAX);
  pc_endjinn_pvr::texture_load_ex(argb1555, texture, 2u, 2u,
                                  PVR_TXRLOAD_16BPP);

  pc_endjinn_pvr::QueuedPrimitive primitive{};
  primitive.textured = true;
  primitive.texture = texture;
  primitive.texture_format = PVR_TXRFMT_ARGB1555;
  primitive.texture_width = 2u;
  primitive.texture_height = 2u;
  pc_endjinn_pvr::DecodedTexture decoded;
  assert(pc_endjinn_pvr::decode_texture(primitive, decoded));
  assert(!decoded.indexed && decoded.mips.size() == 1u);
  assert(decoded.mips[0].pixels[3] == 0xffu);
  assert(decoded.mips[0].pixels[7] == 0xffu);
  assert(decoded.mips[0].pixels[11] == 0x00u);

  const uint8_t pal4[] = {0x10u, 0x32u};
  pvr_ptr_t indices = pc_endjinn_pvr::texture_alloc(sizeof(pal4));
  pc_endjinn_pvr::texture_load_ex(pal4, indices, 2u, 2u,
                                  PVR_TXRLOAD_4BPP);
  primitive.texture = indices;
  primitive.texture_format =
      PVR_TXRFMT_PAL4BPP | PVR_TXRFMT_4BPP_PAL(3u);
  assert(pc_endjinn_pvr::decode_texture(primitive, decoded));
  assert(decoded.indexed && decoded.palette_base == 48u);
  assert((decoded.mips[0].pixels == std::vector<uint8_t>{0u, 1u, 2u, 3u}));

  pc_endjinn_pvr::palette_format(PVR_PAL_ARGB8888);
  pc_endjinn_pvr::palette_entry(48u, 0x00ffffffu);
  pc_endjinn_pvr::palette_entry(49u, 0x80ffc010u);
  const auto palette = pc_endjinn_pvr::palette_rgba();
  assert((palette[48u] >> 24u) == 0u);
  assert((palette[49u] >> 24u) == 0x80u);
  assert((palette[49u] & 0x00ffffffu) == 0x0010c0ffu);

  std::vector<uint8_t> pal8_vq(2048u + 8u, 0xffu);
  for (uint8_t i = 0u; i < 8u; i++) {
    pal8_vq[i] = i;
    pal8_vq[2048u + i] = 0u;
  }
  pvr_ptr_t compressed = pc_endjinn_pvr::texture_alloc(pal8_vq.size());
  pc_endjinn_pvr::texture_load(pal8_vq.data(), compressed, pal8_vq.size());
  primitive.texture = compressed;
  primitive.texture_format = PVR_TXRFMT_PAL8BPP | PVR_TXRFMT_VQ_ENABLE;
  primitive.texture_width = 8u;
  primitive.texture_height = 8u;
  assert(pc_endjinn_pvr::decode_texture(primitive, decoded));
  assert(decoded.indexed && decoded.mips[0].pixels.size() == 64u);

  pc_endjinn_pvr::texture_free(compressed);
  pc_endjinn_pvr::texture_free(indices);
  pc_endjinn_pvr::texture_free(texture);
  return 0;
}
