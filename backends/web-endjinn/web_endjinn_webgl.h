#ifndef WEB_ENDJINN_WEBGL_H
#define WEB_ENDJINN_WEBGL_H

#include <kos.h>

namespace web_endjinn {

vid_mode_t *video_mode();
uint64_t timer_ns_gettime64();
void vid_set_mode(vid_display_mode_generic_t mode, vid_pixel_mode_t pixel_mode);
void pvr_init(const pvr_init_params_t *params);
void pvr_shutdown();
void pvr_set_bg_color(float r, float g, float b);
void pvr_scene_begin();
void pvr_scene_finish();

}  // namespace web_endjinn

#endif
