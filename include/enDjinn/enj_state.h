#ifndef ENJ_STATE_H
#define ENJ_STATE_H

#include <dc/pvr.h>
#include <dc/video.h>
#include <enDjinn/enj_ctrlr.h>
#include <enDjinn/enj_mode.h>

typedef struct enj_state_s {
  union {
    struct {
      uint32_t initialized : 1;
      uint32_t started : 1;
      uint32_t end_mode : 1;
      uint32_t shut_down : 1;
      uint32_t soft_reset_enabled : 1;
      uint32_t : 27;
    };
    uint32_t raw;
  } flags;
  uint32_t exit_pattern;
  int soft_reset_target_index;
  struct {
    vid_display_mode_generic_t display_mode;
    vid_pixel_mode_t pixel_mode;
    pvr_init_params_t pvr_params;
    union {
      struct {
        uint32_t a : 8;
        uint32_t r : 8;
        uint32_t g : 8;
        uint32_t b : 8;
      };
      uint32_t raw;
    } bg_color;
  } video;
} enj_state_t;

enj_state_t *enj_state_get(void);

void enj_state_set_soft_reset(uint32_t pattern);

void enj_state_defaults(void);
int enj_startup();
void enj_run(void);
void enj_shutdown_flag(void);

#endif // ENJ_STATE_H
