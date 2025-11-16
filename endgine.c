/**
 * enDgine - A gameplay loop driver for the Sega Dreamcast
 * Implementation
 */

#include "endgine.h"
#include <kos.h>
#include <stdbool.h>

/* Internal state */
static struct {
    bool running;
    uint64_t start_time;
    uint64_t last_frame_time;
    uint32_t frame_count;
    float current_fps;
    float fps_timer;
    uint32_t target_fps;
} endgine_state = {
    .running = false,
    .start_time = 0,
    .last_frame_time = 0,
    .frame_count = 0,
    .current_fps = 0.0f,
    .fps_timer = 0.0f,
    .target_fps = 60
};

/**
 * Get current time in microseconds
 */
static uint64_t get_time_us(void) {
    return timer_us_gettime64();
}

/**
 * Calculate delta time between frames
 */
static float calculate_delta_time(uint64_t current_time) {
    float delta = (current_time - endgine_state.last_frame_time) / 1000000.0f;
    endgine_state.last_frame_time = current_time;
    return delta;
}

/**
 * Update FPS counter
 */
static void update_fps_counter(float delta_time) {
    endgine_state.frame_count++;
    endgine_state.fps_timer += delta_time;
    
    /* Update FPS every second */
    if (endgine_state.fps_timer >= 1.0f) {
        endgine_state.current_fps = endgine_state.frame_count / endgine_state.fps_timer;
        endgine_state.frame_count = 0;
        endgine_state.fps_timer = 0.0f;
    }
}

/**
 * Frame rate limiting
 */
static void limit_frame_rate(uint32_t target_fps) {
    if (target_fps == 0) {
        return; /* No frame limiting */
    }
    
    uint64_t frame_time_us = 1000000 / target_fps;
    uint64_t current_time = get_time_us();
    uint64_t elapsed = current_time - endgine_state.last_frame_time;
    
    if (elapsed < frame_time_us) {
        uint64_t sleep_time = frame_time_us - elapsed;
        thd_sleep(sleep_time / 1000); /* Convert to milliseconds */
    }
}

int endgine_run(const endgine_config_t *config) {
    if (!config) {
        return -1;
    }
    
    /* Initialize KallistiOS subsystems */
    pvr_init_defaults();
    
    /* Store target FPS */
    endgine_state.target_fps = config->target_fps > 0 ? config->target_fps : 60;
    
    /* Call user initialization */
    if (config->init) {
        int result = config->init();
        if (result != 0) {
            return result;
        }
    }
    
    /* Initialize timing */
    endgine_state.start_time = get_time_us();
    endgine_state.last_frame_time = endgine_state.start_time;
    endgine_state.running = true;
    
    /* Main game loop */
    while (endgine_state.running) {
        uint64_t current_time = get_time_us();
        float delta_time = calculate_delta_time(current_time);
        
        /* Update FPS counter */
        update_fps_counter(delta_time);
        
        /* Check for exit input (Start button on controller) */
        maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        if (cont) {
            cont_state_t *state = (cont_state_t *)maple_dev_status(cont);
            if (state && (state->buttons & CONT_START)) {
                endgine_state.running = false;
                break;
            }
        }
        
        /* Call user update */
        if (config->update) {
            config->update(delta_time);
        }
        
        /* Call user render */
        if (config->render) {
            pvr_wait_ready();
            pvr_scene_begin();
            config->render();
            pvr_scene_finish();
        }
        
        /* Frame rate limiting */
        limit_frame_rate(endgine_state.target_fps);
    }
    
    /* Call user cleanup */
    if (config->cleanup) {
        config->cleanup();
    }
    
    /* Shutdown KallistiOS subsystems */
    pvr_shutdown();
    
    return 0;
}

void endgine_stop(void) {
    endgine_state.running = false;
}

float endgine_get_fps(void) {
    return endgine_state.current_fps;
}

float endgine_get_time(void) {
    uint64_t current_time = get_time_us();
    return (current_time - endgine_state.start_time) / 1000000.0f;
}
