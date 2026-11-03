#ifndef ENJ_STATE_H
#define ENJ_STATE_H

#include <kos.h>
#include <enDjinn/enj_api.h>
#include <enDjinn/enj_ctrl.h>
#include <enDjinn/enj_mode.h>
#include <enDjinn/enj_types.h>

ENJ_BEGIN_DECLS

typedef struct enj_state_s {
  union {
    struct {
      uint32_t initialized : 1;
      uint32_t started : 1;
      uint32_t end_mode : 1;
      uint32_t shut_down : 1;
      uint32_t soft_reset_enabled : 1;
      uint32_t no_snd_init : 1;   /**< startup leaves the AICA to the application */
      uint32_t no_input_init : 1; /**< startup does not set up enDjinn's controllers and rumble */
      uint32_t : 25;
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

/** Set the enDjinn state to default values. Call before changing state fields. */
void enj_state_init_defaults(void);

/** Initialize enDjinn subsystems based on the current state
 *
 * Sets the video mode, initializes the PVR, the injected quick font, the
 * controllers and rumble (unless flags.no_input_init) and the sound driver
 * (unless flags.no_snd_init).
 * @return 0 on success, -1 on failure, including a failed pvr_init()
 */
int enj_state_startup(void);

/** Main enDjinn loop. Runs until shutdown is signaled and then cleans up. */
void enj_state_run(void);

/** Flag enDjinn to shut down at the next opportunity
 * @param __unused Unused parameter, having this allows usage as enj_mode update
 * function
 */
void enj_state_flag_shutdown(void *__unused);

/** Whether the PVR renders with FSAA (twice as wide, filtered down to the
 * screen): as enj_state_startup started it or enj_video_set_fsaa last set it. */
int enj_video_fsaa(void);

/** Render coordinates per screen pixel across: 2 with FSAA, 1 without. Code
 * that switches FSAA at runtime scales by this; ENJ_XSCALE is the build-time
 * value it starts with. */
int enj_video_xscale(void);

/** Switch FSAA on or off while running, by restarting the PVR with the
 * current parameters and the new setting.
 *
 * Every video memory allocation is lost. enDjinn reloads its own (the
 * injected quick font) and then calls the reset callback, so the application
 * reloads its textures and anything else in video memory. Call between
 * frames, with no scene open: a scene load is a good place.
 * @param enabled 1 for FSAA, 0 without
 * @return 0 on success or when nothing changes, -1 if pvr_init() failed
 */
int enj_video_set_fsaa(int enabled);

/** The function enj_video_set_fsaa calls after it restarted the PVR, with
 * data; NULL for none. */
void enj_video_set_reset_callback(void (*callback)(void *data), void *data);

ENJ_END_DECLS

#endif // ENJ_STATE_H
