#ifndef ENJ_PACING_H
#define ENJ_PACING_H

#include <stdint.h>

#include <enDjinn/enj_api.h>

ENJ_BEGIN_DECLS

/**
 * How enj_state_run() paces its ticks. A tick is one call of the current
 * mode's updater followed by one rendered scene.
 */
typedef enum {
  /** As fast as the PVR allows, or one tick per ENJ_FRAME_RATE deadline when
   * that is set. The default. */
  ENJ_PACING_FREE = 0,
  /** One tick per display refresh. A late frame delays every later tick, so
   * the game slows down while frames are late. */
  ENJ_PACING_STRICT,
  /** One tick per display refresh. After a late frame the next tick runs its
   * updater but shows no new scene (enj_render_hold_frame()), and the tick
   * after it is due on its original refresh again, so game time keeps up. At
   * most every second tick is dropped, and a stall of more than a few
   * refreshes (a load) starts the schedule again instead. */
  ENJ_PACING_DROP_LATE,
  /** One tick per display refresh, a new scene on every second tick only: a
   * 30 fps picture on 60 Hz video, with the updater still called every
   * refresh. A late frame delays the schedule as in ENJ_PACING_STRICT. */
  ENJ_PACING_HALF,
} enj_pacing_t;

typedef struct {
  uint32_t ticks;    /**< updater calls */
  uint32_t rendered; /**< ticks whose scene was rendered */
  uint32_t dropped;  /**< ticks shown no new scene by the pacing */
  uint32_t late;     /**< ticks that finished after their refresh */
} enj_pacing_stats_t;

/**
 * Select the pacing. Takes effect at the next tick and may be changed at any
 * time, for example from an options menu; the refresh schedule starts again
 * from the next refresh.
 *
 * The non-free modes replace the ENJ_FRAME_RATE deadline and assume the
 * display runs at about 60 Hz. A game that paces its own loop (built with
 * ENJ_FRAME_RATE=0 and waiting on vblank itself) keeps ENJ_PACING_FREE.
 */
void enj_pacing_set(enj_pacing_t mode);

/** The current pacing. */
enj_pacing_t enj_pacing_get(void);

/**
 * Whether the tick running now will show no new scene: set before the
 * updater is called, so the updater can skip building render lists and keep
 * any per-scene bookkeeping (interpolation, once-per-picture effects) in step.
 * Game state must still advance; only the picture is dropped. A hold the game
 * requests itself with enj_render_hold_frame() is not reported here.
 */
int enj_pacing_scene_dropped(void);

/**
 * The tick counter: 0 for the first tick after enj_pacing_set(), then one more
 * per updater call. In ENJ_PACING_HALF the even ticks are rendered.
 */
uint32_t enj_pacing_tick(void);

/** Counters since startup, for profiling or an on-screen display. */
void enj_pacing_stats(enj_pacing_stats_t *out);

ENJ_END_DECLS

#endif // ENJ_PACING_H
