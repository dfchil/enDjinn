#include <enDjinn/enj_enDjinn.h>
#include <enDjinn/enj_web_render.h>

#include <type_traits>

static_assert(std::is_same_v<decltype(&enj_mode_pop), enj_mode_t *(*)(void)>);
static_assert(std::is_same_v<decltype(&enj_bitmap_get),
                             int (*)(const enj_bitmap_t *, int, int)>);

int main() {
  enj_mode_t mode{};
  mode.name[0] = '\0';
  return mode.name[0];
}
