#ifndef ENJ_RENDER_H
#define ENJ_RENDER_H

#include <kos.h>
#include <enDjinn/enj_api.h>
#include <enDjinn/enj_mode.h>

ENJ_BEGIN_DECLS

/**
 * Add a rendering item to the specified render list.
 * @param renderlist Render list to add the item to
 * @param renderer Pointer to the rendering function to be called when doing the
 * specified render list
 * @param data Pointer to data to pass to the rendering function
 *
 * @note enDjinn stores data as a pointer; it does not copy the payload. The
 * pointed-to object must remain valid until the callback runs later in the
 * current frame. Invalid list values and NULL callbacks are ignored.
 */
void enj_render_list_add(pvr_list_t renderlist, void (*renderer)(void *data),
                        void *data);

/**
 * Empty the render lists: what was queued for this frame is not drawn.
 *
 * For a frame abandoned half way, as a scene load can leave one. Whatever the
 * dropped items pointed to may go right after this call.
 */
void enj_render_lists_clear(void);

/**
 * Advance to the next frame, calling the current mode's updater.
 * @param updater Pointer to the current mode
 *
 * @note This function is intended to be called by the main loop in the
 * enj_state_run function.
 */
void enj_render_next_frame(enj_mode_t *updater);

/**
 * Submit one PVR scene from the queued render lists, then empty them.
 *
 * Waits for the PVR, begins the scene, calls every queued renderer between
 * pvr_list_begin() and pvr_list_finish() of its list, finishes the scene and
 * runs the post-render callback. Lists with nothing queued, and lists that
 * enj_state_startup() gave no bins, are not opened.
 *
 * enj_render_next_frame() calls this after the mode updater. An application
 * that runs its own frame loop can queue renderers with enj_render_list_add()
 * and call this directly instead of enj_state_run().
 */
void enj_render_scene(void);

/**
 * Show the previous frame again instead of this frame's render lists.
 *
 * Call it from a mode updater (or before enj_render_scene() in an
 * application's own loop). The next enj_render_scene() then discards the
 * queued render lists, presents the last submitted scene once more and runs
 * the post-render callback. For a game that holds its picture for a display
 * refresh, for example one that shows each 30 Hz step on two 60 Hz refreshes
 * and must not redraw the first refresh's state.
 *
 * Dreamcast: no scene is submitted; the PVR keeps displaying the last frame.
 * pc-enDjinn and web-enDjinn: the last scene's primitives are drawn again.
 * Textures it used stay valid even if they were freed since; texture data
 * that was reloaded in between is shown as it is now.
 */
void enj_render_hold_frame(void);

/**
 * What an application does around the lists of a submitted scene. Each may be
 * NULL. enj_render_list_add() queues draws; these are for what belongs to the
 * scene or to a list as a whole.
 */
typedef struct enj_render_hooks_s {
  /** After pvr_scene_begin(), before the first list opens: the frame's setup,
   * once the updater has queued everything. */
  void (*scene_begin)(void *data);
  /** After pvr_list_begin(list), before the list's first renderer: each list
   * is a pass over the frame of its own, and starts from what this sets. */
  void (*list_begin)(pvr_list_t list, void *data);
  /** After the last list closed, before pvr_scene_finish(): what has to reach
   * the PVR before it renders, once all the renderers ran (per-frame
   * registers such as the fog table, for instance). */
  void (*lists_done)(void *data);
  void *data; /**< Passed to each of them */
} enj_render_hooks_t;

/**
 * Set the scene's hooks. A held frame (enj_render_hold_frame()) calls none.
 * @param hooks The hooks, copied; NULL for none
 */
void enj_render_hooks_set(const enj_render_hooks_t *hooks);

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
 * Print the sizes of the render lists for debugging
 */
void enj_render_print_list_sizes(void);

ENJ_END_DECLS

#endif // ENJ_RENDER_H
