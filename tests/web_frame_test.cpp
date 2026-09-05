#include "../backends/host-common/host_pvr.h"
#include "../backends/web-endjinn/web_endjinn_frame.h"

#include <cassert>

namespace {

void submit_triangle(bool alpha_cutout, float x) {
  pvr_poly_cxt_t context{};
  pvr_poly_cxt_col(&context, PVR_LIST_OP_POLY);
  context.gen.culling = PVR_CULLING_NONE;
  context.gen.alpha = alpha_cutout ? 1u : 0u;
  auto *header = static_cast<pvr_poly_hdr_t *>(enj_host_pvr::dr_target());
  pvr_poly_compile(header, &context);
  enj_host_pvr::dr_commit(header);

  for (int i = 0; i < 3; i++) {
    auto *vertex = static_cast<pvr_vertex_t *>(enj_host_pvr::dr_target());
    *vertex = {i == 2 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX,
               x + static_cast<float>(i * 8),
               10.0f + static_cast<float>((i & 1) * 8),
               0.5f, 0.0f, 0.0f, 0xffffffffu, 0u};
    enj_host_pvr::dr_commit(vertex);
  }
}

}  // namespace

int main() {
  enj_host_pvr::scene_begin();
  enj_host_pvr::list_begin(PVR_LIST_OP_POLY);
  submit_triangle(false, 10.0f);
  submit_triangle(true, 40.0f);

  const auto &frame = web_endjinn::build_frame({640, 480}, false, true);
  assert(frame.batches.size() == 2u);
  assert(!frame.batches[0].alpha_cutout);
  assert(frame.batches[1].alpha_cutout);
  assert(frame.batches[0].vertex_count == 3u);
  assert(frame.batches[1].vertex_count == 3u);
  return 0;
}
