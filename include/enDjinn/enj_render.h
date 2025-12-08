#ifndef ENDJINN_RENDER_H
#define ENDJINN_RENDER_H

#include <enDjinn/enj_enDjinn.h>

typedef void (*enj_render_post_call)(void *data);

/**
 * Add a rendering item to the specified renderlist
 * @param renderlist Renderlist to add the item to
 * @param renderer Pointer to the rendering function to be called when doing the
 * specified renderlist
 * @param data Pointer to data to pass to the rendering function
 */
void enj_renderlist_add(pvr_list_t renderlist, void (*renderer)(void *data),
                        void *data);

/**
 * Advance to the next frame, calling the current mode's updater.
 * @param updater Pointer to the current mode
 *
 * @note This function is intended to be called by the main loop in the
 * enj_run function.
 */
void enj_next_frame(enj_mode_t *updater);

/**
 * Set a callback to be called after each rendering iteration
 * @param cleanup Pointer to the callback function
 * @param data Pointer to data to pass to the callback
 */
void enj_render_post_callback(enj_render_post_call cleanup, void *data);

/**
 * Set the palette mode to switch to at the end of the frame
 * @param mode Palette mode to switch to
 *
 */
void enj_render_set_palette_mode(pvr_palfmt_t mode);

/**
 * Print the sizes of the renderlists for debugging
 */
void enj_print_renderlist_sizes(void);

#endif // ENDJINN_RENDER_H