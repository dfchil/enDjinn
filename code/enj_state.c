#include <dc/maple/purupuru.h>
#include <dc/sound/sound.h>
#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>
#include <kos.h>

#ifdef ENJ_INJECT_QFONT
#include <enDjinn/enj_qfont.h>
#endif

#ifdef ENJ_DEBUG
#include <arch/gdb.h>
#endif

#ifdef DCPROF
#include "../enDjinn/profilers/dcprof/profiler.h"
#endif
#ifdef ENJ_DEBUG
#include <dc/perf_monitor.h>
#endif

#include <enDjinn/embeds/enj_logo_header.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

alignas(32) static enj_state_t state = {0};
enj_state_t *enj_state_get(void) { return &state; }

static inline void _vmu_splash_screen(void) {
  /** set vmu screens */
  vmufb_t vmufb;
  vmufb_clear(&vmufb);
  vmufb_paint_area(&vmufb, 16, 0, 32, 32, enj_logo_bitmap_header);
  vmufb_print_string_into(&vmufb, NULL, 1, 1, 48, 32, 0, "enDjinn");
  vmufb_print_string_into(&vmufb, NULL, 9, 6, 48, 32, 0, "r");
  vmufb_print_string_into(&vmufb, NULL, 8, 12, 48, 32, 0, "i");
  vmufb_print_string_into(&vmufb, NULL, 9, 17, 48, 32, 0, "v");
  vmufb_print_string_into(&vmufb, NULL, 9, 22, 48, 32, 0, "e");
  vmufb_print_string_into(&vmufb, NULL, 9, 27, 48, 32, 0, "n");

  for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
    maple_device_t *vmulcd = enj_maple_port_type(i, MAPLE_FUNC_LCD);
    if (vmulcd) {
      vmufb_present(&vmufb, vmulcd);
    }
  }
}


void enj_state_init_defaults(void) {
#ifdef ENJ_DEBUG
  gdb_init();
  ENJ_DEBUG_PRINT("ENJ_CBASEPATH %s\n", ENJ_CBASEPATH);
#endif

  _vmu_splash_screen();

  state.flags.raw = 0;
  state.flags.initialized = 1;
  state.flags.soft_reset_enabled = 1;
  state.soft_reset_target_index = -1;
  state.exit_pattern = ((enj_ctrlr_state_t){.button = {.START = ENJ_BUTTON_DOWN,
                                                       .A = ENJ_BUTTON_DOWN,
                                                       .B = ENJ_BUTTON_DOWN,
                                                       .X = ENJ_BUTTON_DOWN,
                                                       .Y = ENJ_BUTTON_DOWN}})
                           .button.raw;

  state.video.pvr_params = (pvr_init_params_t){
      .opb_sizes = {PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16,
                    PVR_BINSIZE_16, PVR_BINSIZE_16},
      .vertex_buf_size = 1024 * 1024,
      .dma_enabled = 0,
      .fsaa_enabled = ENJ_SUPERSAMPLING,
      .autosort_disabled = 1,
      .opb_overflow_count = 2,
      .vbuf_doublebuf_disabled = 0,
  };
  state.video.display_mode = DM_640x480;
  state.video.pixel_mode = PM_RGB888P;
  state.video.bg_color.raw = 0x00000000;
}

void enj_state_soft_reset_set(uint32_t pattern) {
  state.exit_pattern = pattern;
}

void enj_state_flag_shutdown(void *__unused) { state.flags.shut_down = 1; }

void enj_ctrl_init_local_devices(void);
void enj_rumble_init_local_devices(void);
void enj_rumble_update(void);

int enj_state_startup() {
  enj_state_t *state = enj_state_get();

  if (!state->flags.initialized) {
    ENJ_DEBUG_PRINT(
        "enDjinn not initialized! Call enj_state_init_defaults() first.\n");
    return -1;
  }

  vid_set_mode(state->video.display_mode, state->video.pixel_mode);
  pvr_init(&state->video.pvr_params);
  pvr_set_bg_color(state->video.bg_color.r / 255.0f,
                   state->video.bg_color.g / 255.0f,
                   state->video.bg_color.b / 255.0f);

#ifdef ENJ_DEBUG
  perf_monitor_init(PMCR_OPERAND_CACHE_READ_MISS_MODE,
                    PMCR_INSTRUCTION_CACHE_MISS_MODE);
#endif

#ifdef DCPROF
  profiler_init("/pc/gmon.out");
  profiler_start();
#endif

#ifdef ENJ_INJECT_QFONT
  enj_qfont_init();
#endif

  state->flags.started = 1;

  enj_ctrl_init_local_devices();
  enj_rumble_init_local_devices();
  snd_init();

  return 0;
}

void enj_state_run(void) {
  if (enj_mode_get_current_index() < 0) {
    ENJ_DEBUG_PRINT(
        "No mode pushed! Call enj_mode_push() before enj_state_run().\n");
    return;
  }
  enj_state_t *state = enj_state_get();
  if (!state->flags.started) {
    ENJ_DEBUG_PRINT("enDjinn not started! Call enj_state_startup() before "
                    "enj_state_run().\n");
    return;
  }
  enj_ctrlr_state_t **cstates = enj_ctrl_get_states();

  while (1) {
    if (state->flags.shut_down) {
      break;
    }
    if (state->flags.end_mode) {
      if (enj_mode_pop() == NULL) {
        // no more modes, exit
        break;
      }
      state->flags.end_mode = 0;
    }
    enj_ctrl_map_states();
    if (state->flags.soft_reset_enabled && !(enj_mode_get()->no_soft_reset)) {
      /* check for controller exit patterns */
      for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
        if (cstates[i] && enj_ctrlr_button_combo_raw(cstates[i]->button.raw,
                                                     state->exit_pattern)) {
          // prevent re-triggering
          cstates[i]->button.raw &= ~(state->exit_pattern << 1);

          int mode_index = enj_mode_get_current_index();
          if (mode_index == 0) {
            // we're at the base mode, just shut down
            enj_state_flag_shutdown(NULL);
            break;
          }
          if (state->soft_reset_target_index != -1 &&
              (mode_index != state->soft_reset_target_index)) {
            enj_mode_cut_to_soft_reset_target();
          } else {
#ifdef ENJ_DEBUG
            enj_mode_t *from_mode = enj_mode_get();
#endif
            enj_mode_pop();
#ifdef ENJ_DEBUG
            enj_mode_t *nxt_mode = enj_mode_get();
            ENJ_DEBUG_PRINT("Exiting from mode '%s':%d to mode '%s:%d'\n",
                            from_mode->name, mode_index, nxt_mode->name,
                            enj_mode_get_current_index());
#endif
          }
        }
      }
    }
    enj_render_next_frame(enj_mode_get());
    enj_rumble_update();
  }
#ifdef DCPROF
  profiler_stop();
  profiler_clean_up();
#endif
  pvr_shutdown();

#ifdef ENJ_DEBUG
  perf_monitor_print(stdout);

  FILE *stats_out = fopen(ENJ_CBASEPATH "/pstats.txt", "a");
  if (stats_out != NULL) {
    perf_monitor_print(stats_out);
    fclose(stats_out);
  }
#endif

  // clear vmu screens and stop rumblers
  vmufb_t *vmufb = NULL;
  for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
    maple_device_t *vmulcd = enj_maple_port_type(i, MAPLE_FUNC_LCD);
    if (vmulcd) {
      if (vmufb == NULL) {
        vmufb = memalign(32, sizeof(vmufb_t));
        vmufb_clear(vmufb);
      }
      vmufb_present(vmufb, vmulcd);
    }
    maple_device_t *rumbler = enj_maple_port_type(i, MAPLE_FUNC_PURUPURU);
    if (rumbler) {
      purupuru_rumble_raw(rumbler, (purupuru_effect_t){.motor = 1}.raw);
    }
  }
  if (vmufb != NULL) {
    free(vmufb);
    usleep(100000); // allow time for VMU to update
  }

#ifdef RELEASEBUILD
  arch_set_exit_path(ARCH_EXIT_REBOOT);
#endif
}
