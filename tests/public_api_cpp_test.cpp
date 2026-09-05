#include <enDjinn/enj_enDjinn.h>
#include <pc_endjinn/capture.h>

#include <type_traits>

static_assert(std::is_same_v<decltype(&enj_mode_pop), enj_mode_t *(*)(void)>);
static_assert(std::is_same_v<decltype(&enj_bitmap_get),
                             int (*)(const enj_bitmap_t *, int, int)>);
static_assert(std::is_same_v<decltype(&enj_pc_capture_next_frame),
                             bool (*)(const char *)>);
static_assert(std::is_same_v<decltype(&enj_pc_presented_frame_count),
                             uint64_t (*)(void)>);

int main() {
  enj_mode_t mode{};
  mode.name[0] = '\0';
  return mode.name[0];
}
