#include <kos.h>

#include "pc_endjinn_pvr.h"
#include "pc_endjinn_vulkan.h"

extern "C" {

 void* memalign(size_t alignment, size_t size) {
    void *ptr = malloc(size + alignment - 1 + sizeof(void*));

    if (ptr == NULL) {
        return NULL; // Allocation failed
    }
    return ptr;
}

vid_mode_t *vid_mode = pc_endjinn::video_mode();

uint64_t timer_ns_gettime64(void) {
  return pc_endjinn::timer_ns_gettime64();
}

void vid_border_color(uint8_t r, uint8_t g, uint8_t b) {
  pc_endjinn::vid_border_color(r, g, b);
}

void vid_set_mode(vid_display_mode_generic_t display_mode,
                  vid_pixel_mode_t pixel_mode) {
  pc_endjinn::vid_set_mode(display_mode, pixel_mode);
}

void pvr_init(const pvr_init_params_t *params) {
  pc_endjinn::pvr_init(params);
}

void pvr_shutdown(void) { pc_endjinn::pvr_shutdown(); }

void pvr_set_bg_color(float r, float g, float b) {
  pc_endjinn::pvr_set_bg_color(r, g, b);
}

void pvr_wait_ready(void) { pc_endjinn::pvr_wait_ready(); }
void pvr_scene_begin(void) { pc_endjinn::pvr_scene_begin(); }
void pvr_scene_finish(void) { pc_endjinn::pvr_scene_finish(); }
uint64_t pc_endjinn_pvr_presented_frame_count(void) {
  return pc_endjinn::pvr_presented_frame_count();
}
void pc_endjinn_pvr_request_current_scene_screenshot(const char *path) {
  pc_endjinn::pvr_request_current_scene_screenshot(path);
}
void pvr_list_begin(pvr_list_t list) { pc_endjinn::pvr_list_begin(list); }
void pvr_list_finish(void) { pc_endjinn::pvr_list_finish(); }
void pvr_wait_render_done(void) { pc_endjinn::pvr_wait_render_done(); }

void pvr_set_pal_format(pvr_palfmt_t mode) {
  pc_endjinn::pvr_set_pal_format(mode);
}

void pvr_set_pal_entry(uint32_t index, uint32_t value) {
  pc_endjinn_pvr::palette_entry(index, value);
}

void *pvr_mem_malloc(size_t size) { return pc_endjinn_pvr::texture_alloc(size); }
void pvr_mem_free(pvr_ptr_t ptr) { pc_endjinn_pvr::texture_free(ptr); }

void pvr_txr_load(const void *src, pvr_ptr_t dst, size_t count) {
  pc_endjinn_pvr::texture_load(src, dst, count);
}

void pvr_txr_load_ex(const void *src, pvr_ptr_t dst, uint32_t width,
                     uint32_t height, uint32_t flags) {
  pc_endjinn_pvr::texture_load_ex(src, dst, width, height, flags);
}

void pvr_fog_table_color(float a, float r, float g, float b) {
  pc_endjinn::pvr_fog_table_color(a, r, g, b);
}

void pvr_fog_table_linear(float start, float end) {
  pc_endjinn::pvr_fog_table_linear(start, end);
}

void *pvr_dr_target(void) { return pc_endjinn::pvr_dr_target(); }
void pvr_dr_commit(void *ptr) { pc_endjinn::pvr_dr_commit(ptr); }

}  // extern "C"
