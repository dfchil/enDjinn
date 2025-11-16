/**
 * enDJinn - A gameplay loop driver for the Sega Dreamcast
 * 
 * Provides a simple, clean API for creating games on the Dreamcast
 * using KallistiOS.
 */

#ifndef ENDJINN_H
#define ENDJINN_H

#include <stdint.h>

/* Callback function types */
typedef int (*endgine_init_func)(void);
typedef void (*endgine_update_func)(float delta_time);
typedef void (*endgine_render_func)(void);
typedef void (*endgine_cleanup_func)(void);

/* Game loop configuration */
typedef struct {
    endgine_init_func init;       /* Called once at startup */
    endgine_update_func update;   /* Called every frame with delta time */
    endgine_render_func render;   /* Called every frame for rendering */
    endgine_cleanup_func cleanup; /* Called once at shutdown */
    uint32_t target_fps;          /* Target frames per second (0 = unlimited) */
} endgine_config_t;

/**
 * Initialize and run the game loop
 * 
 * @param config Configuration structure with callbacks
 * @return 0 on success, non-zero on error
 */
int endgine_run(const endgine_config_t *config);

/**
 * Request the game loop to stop
 * Can be called from within update or render callbacks
 */
void endgine_stop(void);

/**
 * Get the current frame rate
 * 
 * @return Current FPS
 */
float endgine_get_fps(void);

/**
 * Get total elapsed time since start
 * 
 * @return Time in seconds
 */
float endgine_get_time(void);

#endif /* ENDJINN_H */
