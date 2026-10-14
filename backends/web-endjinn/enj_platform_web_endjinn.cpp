#include <kos.h>

#include "../host-common/host_pvr.h"
#include "web_endjinn_webgl.h"

extern "C" {

vid_mode_t *vid_mode = web_endjinn::video_mode();

uint64_t timer_ns_gettime64(void) {
  return web_endjinn::timer_ns_gettime64();
}

void vid_border_color(uint8_t, uint8_t, uint8_t) {}

void vid_set_mode(vid_display_mode_generic_t mode,
                  vid_pixel_mode_t pixel_mode) {
  web_endjinn::vid_set_mode(mode, pixel_mode);
}

void pvr_init(const pvr_init_params_t *params) {
  web_endjinn::pvr_init(params);
}
void pvr_shutdown(void) { web_endjinn::pvr_shutdown(); }
void pvr_set_bg_color(float r, float g, float b) {
  web_endjinn::pvr_set_bg_color(r, g, b);
}
void pvr_wait_ready(void) {}
void pvr_scene_begin(void) { web_endjinn::pvr_scene_begin(); }
void pvr_scene_finish(void) { web_endjinn::pvr_scene_finish(); }
void pvr_list_begin(pvr_list_t list) { enj_host_pvr::list_begin(list); }
void pvr_list_finish(void) {}
void pvr_wait_render_done(void) {}

void pvr_set_pal_format(pvr_palfmt_t mode) {
  enj_host_pvr::palette_format(mode);
}
void pvr_set_pal_entry(uint32_t index, uint32_t value) {
  enj_host_pvr::palette_entry(index, value);
}
void *pvr_mem_malloc(size_t size) {
  return enj_host_pvr::texture_alloc(size);
}
void pvr_mem_free(pvr_ptr_t ptr) { enj_host_pvr::texture_free(ptr); }
void pvr_txr_load(const void *source, pvr_ptr_t destination, size_t count) {
  enj_host_pvr::texture_load(source, destination, count);
}
void pvr_txr_load_ex(const void *source, pvr_ptr_t destination,
                     uint32_t width, uint32_t height, uint32_t flags) {
  enj_host_pvr::texture_load_ex(source, destination, width, height, flags);
}
void pvr_fog_table_color(float, float, float, float) {}
void pvr_fog_table_linear(float, float) {}
void *pvr_dr_target(void) { return enj_host_pvr::dr_target(); }
void pvr_dr_commit(void *ptr) { enj_host_pvr::dr_commit(ptr); }

}  // extern "C"
