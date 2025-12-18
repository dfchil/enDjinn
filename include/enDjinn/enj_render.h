#ifndef ENDJINN_RENDER_H
#define ENDJINN_RENDER_H

#include <dc/pvr.h>
#include <dc/pvr/pvr_pal.h>
#include <enDjinn/enj_mode.h>

/**
 * Add a rendering item to the specified renderlist
 * @param renderlist Renderlist to add the item to
 * @param renderer Pointer to the rendering function to be called when doing the
 * specified renderlist
 * @param data Pointer to data to pass to the rendering function
 */
void enj_render_list_add(pvr_list_t renderlist, void (*renderer)(void *data),
                        void *data);

/**
 * Advance to the next frame, calling the current mode's updater.
 * @param updater Pointer to the current mode
 *
 * @note This function is intended to be called by the main loop in the
 * enj_state_run function.
 */
void enj_render_next_frame(enj_mode_t *updater);

/**
 * Set a callback to be called after each rendering iteration
 * @param render_post_call Pointer to the callback function
 * @param data Pointer to data to pass to the callback
 */
void enj_render_post_callback_set(void (*render_post_call)(void *), void *data);

/**
 * Set the palette mode to switch to at the end of the frame
 * @param mode Palette mode to switch to
 *
 */
void enj_render_palette_mode_set(pvr_palfmt_t mode);

/**
 * Print the sizes of the renderlists for debugging
 */
void enj_render_print_list_sizes(void);

#endif // ENDJINN_RENDER_H