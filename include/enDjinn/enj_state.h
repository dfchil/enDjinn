#ifndef ENJ_STATE_H
#define ENJ_STATE_H

#include <dc/pvr.h>
#include <dc/video.h>
#include <enDjinn/enj_ctrl.h>
#include <enDjinn/enj_mode.h>
#include <enDjinn/enj_types.h>

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
    enj_color_t bg_color;
  } video;
} enj_state_t;

enj_state_t *enj_state_get(void);

/**
 * Set the controller button pattern that triggers a soft reset
 * @param pattern Button pattern to set
 *
 * @note The default pattern is START + A + B + X + Y all pressed down.
 *
 * @note To set the soft reset pattern to START + A:
 * enj_state_soft_reset_set(((enj_ctrlr_state_t){.button = {.START =
 * ENJ_BUTTON_DOWN, .A = ENJ_BUTTON_DOWN}}).button.raw)
 */
void enj_state_soft_reset_set(uint32_t pattern);

/** Set the enDjinn state to default values */
void enj_state_init_defaults(void);

/** Initialize enDjinn subsystems based on the current state
 * @return 0 on success, -1 on failure
 */
int enj_state_startup();

/** 
 * Main enDjinn loop, runs until shutdown is signaled 
 * */
void enj_state_run(void);

/** Flag enDjinn to shut down at the next opportunity
 * @param __unused Unused parameter, having this allows usage as enj_mode update
 * function
 */
void enj_state_flag_shutdown(void *__unused);

#endif // ENJ_STATE_H