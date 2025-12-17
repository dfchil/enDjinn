#include <dc/video.h>
#include <enDjinn/enj_enDjinn.h>
#include <kos.h>
#include <dc/sound/sound.h>

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

KOS_INIT_FLAGS(INIT_DEFAULT);

alignas(32) static enj_state_t state = {0};
enj_state_t* enj_state_get(void) { return &state; }

void enj_state_defaults(void) {
#ifdef ENJ_DEBUG
    gdb_init();
    ENJ_DEBUG_PRINT("ENJ_CBASEPATH %s\n", ENJ_CBASEPATH);
#endif

    state.flags.raw = 0;
    state.flags.initialized = 1;
    state.flags.soft_reset_enabled = 1;
    state.soft_reset_target_index = -1;
    state.exit_pattern = ((enj_ctrlr_state_t){.buttons = {.START = BUTTON_DOWN,
                                                          .A = BUTTON_DOWN,
                                                          .B = BUTTON_DOWN,
                                                          .X = BUTTON_DOWN,
                                                          .Y = BUTTON_DOWN}})
                             .buttons.raw;

    state.video.pvr_params = (pvr_init_params_t){
        {PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16,
         PVR_BINSIZE_16},   // Bin sizes
        1024 * 1024,        // Vertex buffer size
        0,                  // No DMA
        ENJ_SUPERSAMPLING,  // Set horisontal FSAA
        0,                  // Translucent Autosort enabled.
        3,                  // Extra OPBs
        0,                  // No extra PTs
    };
    state.video.display_mode = DM_640x480;
    state.video.pixel_mode = PM_RGB888P;
    state.video.bg_color.raw = 0x00000000;
}

void enj_state_set_soft_reset(uint32_t pattern) {
    state.exit_pattern = pattern;
}

void enj_shutdown_flag(void) { state.flags.shut_down = 1; }

void enj_ctrl_init_local_devices(void);
void enj_rumble_init_local_devices(void);
void enj_rumble_update(void);

int enj_startup() {
    enj_state_t* state = enj_state_get();

    if (!state->flags.initialized) {
        ENJ_DEBUG_PRINT(
            "enDjinn not initialized! Call enj_state_defaults() first.\n");
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

void enj_run(void) {
    if (enj_mode_get_current_index() < 0) {
        ENJ_DEBUG_PRINT(
            "No mode pushed! Call enj_mode_push() before enj_run().\n");
        return;
    }
    enj_state_t* state = enj_state_get();
    if (!state->flags.started) {
        ENJ_DEBUG_PRINT(
            "enDjinn not started! Call enj_startup() before enj_run().\n");
        return;
    }
    enj_ctrlr_state_t** cstates = enj_ctrl_get_states();

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
        if (state->flags.soft_reset_enabled) {
            /* check for controller exit patterns */
            for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
                if (cstates[i] &&
                    enj_ctrlr_button_combo_raw(cstates[i]->buttons.raw,
                                               state->exit_pattern)) {
                    // prevent re-triggering
                    cstates[i]->buttons.raw &= ~(state->exit_pattern << 1);

                    int mode_index = enj_mode_get_current_index();
                    if (mode_index == 0) {
                        // we're at the base mode, just shut down
                        enj_shutdown_flag();
                        break;
                    }
                    if (state->soft_reset_target_index != -1 &&
                        (mode_index != state->soft_reset_target_index)) {
                        enj_mode_cut_to_soft_reset_target();
                    } else {
#ifdef ENJ_DEBUG
                        enj_mode_t* from_mode = enj_mode_get();
#endif
                        enj_mode_pop();
#ifdef ENJ_DEBUG
                        enj_mode_t* nxt_mode = enj_mode_get();
                        ENJ_DEBUG_PRINT(
                            "Exiting from mode '%s':%d to mode '%s:%d'\n",
                            from_mode->name, mode_index, nxt_mode->name,
                            enj_mode_get_current_index());
#endif
                    }
                }
            }
        }
        enj_next_frame(enj_mode_get());
        enj_rumble_update();
    }
#ifdef DCPROF
    profiler_stop();
    profiler_clean_up();
#endif
    pvr_shutdown();

#ifdef ENJ_DEBUG
    perf_monitor_print(stdout);

    FILE* stats_out = fopen(ENJ_CBASEPATH "/pstats.txt", "a");
    if (stats_out != NULL) {
        perf_monitor_print(stats_out);
        fclose(stats_out);
    }
#endif
#ifdef RELEASEBUILD
    arch_set_exit_path(ARCH_EXIT_REBOOT);
#endif
}
