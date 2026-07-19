#include <assert.h>
#include <dc/fmath.h>
#include <enDjinn/enj_enDjinn.h>
#include <pvrtex/file_dctex.h>
#include <sh4zam/shz_sh4zam.h>

static enj_state_t test_state;
static int activations;

enj_state_t *enj_state_get(void) { return &test_state; }
void enj_state_flag_shutdown(void *unused) { (void)unused; }

static void activated(enj_mode_t *previous, enj_mode_t *next) {
  assert(previous != NULL);
  assert(next != NULL);
  activations++;
}

int main(void) {
  float sine;
  float cosine;
  fsincosr(0.0f, &sine, &cosine);
  assert(sine == 0.0f && cosine == 1.0f);
  shz_vec3_t vector = {.x = 1.0f, .y = 2.0f, .z = 3.0f};
  assert(vector.z == 3.0f);

  fDtHeader paletted = {.colors_used = 15,
                        .pvr_type = (5u << 27) | (4u << 3) | 3u};
  assert(sizeof(fDtHeader) == 32);
  assert(fDtIsPalettized(&paletted));
  assert(fDtGetColorsUsed(&paletted) == 16);
  assert(fDtGetPvrWidth(&paletted) == 128);
  assert(fDtGetPvrHeight(&paletted) == 64);

  assert(enj_mode_get() == NULL);
  assert(enj_mode_pop() == NULL);
  enj_mode_set(NULL);
  enj_mode_goto_index(-1);

  enj_mode_t base = {.name = "base", .on_activation_fn = activated};
  enj_mode_t overlay = {.name = "overlay"};
  assert(enj_mode_push(NULL) == 0);
  assert(enj_mode_push(&base) == 1);
  assert(enj_mode_pop() == NULL);
  assert(enj_mode_get() == &base);
  assert(enj_mode_push(&overlay) == 1);
  assert(enj_mode_pop() == &overlay);
  assert(enj_mode_get() == &base);
  assert(activations == 1);

  assert(enj_bitmap_create(0, 8) == NULL);
  assert(enj_bitmap_create(8, -8) == NULL);
  enj_bitmap_t *bitmap = enj_bitmap_create(8, 8);
  assert(bitmap != NULL);
  enj_bitmap_set(bitmap, 3, 4);
  assert(enj_bitmap_get(bitmap, 3, 4) == 1);
  enj_bitmap_destroy(bitmap);
  return 0;
}
