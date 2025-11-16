/*
 * enDgine - Dreamcast Gameplay Loop Driver
 * Main entry point with KallistiOS initialization
 */

#include <kos.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

/* PVR (PowerVR) includes for graphics */
#include <dc/pvr.h>

/* Global flag for running state */
static int running = 1;

/**
 * Initialize the Dreamcast system
 * Sets up video, controllers, and other subsystems
 */
int init_system(void) {
    /* Initialize PVR (PowerVR graphics system) */
    pvr_init_defaults();
    
    /* Initialize controller system */
    maple_device_t *cont;
    cont_state_t *state;
    
    /* Clear the screen to black */
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);
    
    return 0;
}

/**
 * Main game loop
 * Handles input, updates game state, and renders
 */
void game_loop(void) {
    maple_device_t *cont;
    cont_state_t *state;
    
    while(running) {
        /* Get controller state from port A */
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        
        if(cont) {
            state = (cont_state_t *)maple_dev_status(cont);
            
            if(state) {
                /* Check for START button to exit */
                if(state->buttons & CONT_START) {
                    running = 0;
                }
            }
        }
        
        /* Begin PVR frame */
        pvr_wait_ready();
        pvr_scene_begin();
        
        /* Render opaque polygons */
        pvr_list_begin(PVR_LIST_OP_POLY);
        /* Game rendering code would go here */
        pvr_list_finish();
        
        /* Render transparent polygons */
        pvr_list_begin(PVR_LIST_TR_POLY);
        /* Transparent rendering code would go here */
        pvr_list_finish();
        
        /* End the PVR frame */
        pvr_scene_finish();
    }
}

/**
 * Cleanup and shutdown
 */
void shutdown_system(void) {
    /* Shutdown PVR */
    pvr_shutdown();
}

/**
 * Main entry point
 * Initializes KallistiOS and starts the game loop
 */
int main(int argc, char **argv) {
    /* Initialize system */
    if(init_system() != 0) {
        return -1;
    }
    
    /* Run the main game loop */
    game_loop();
    
    /* Cleanup */
    shutdown_system();
    
    return 0;
}
