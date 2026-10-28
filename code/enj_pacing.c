#include <enDjinn/enj_pacing.h>
#include <enDjinn/enj_render.h>
#include <kos.h>

#ifdef __DREAMCAST__
#include <dc/vblank.h>
#include <kos/genwait.h>
#endif

/* A stall longer than this many refreshes is a load or a mode change, not a
 * frame that came out heavy: the schedule starts again instead of dropping. */
#define ENJ_PACING_MAX_BEHIND 3

static enj_pacing_t pacing_mode = ENJ_PACING_FREE;
static enj_pacing_stats_t pacing_stats;
static uint32_t pacing_tick;      /* ticks since the mode was set */
static uint32_t pacing_due;       /* refresh the running tick's scene is due on */
static int pacing_restart = 1;    /* take the schedule from the next refresh */
static int pacing_drop_next;      /* the next tick catches up without a scene */
static int pacing_dropped_now;    /* the running tick shows no new scene */

#ifdef __DREAMCAST__
/* The refresh clock: counted by a vblank handler, which also wakes the loop
 * waiting for a refresh. Added the first time a refresh-locked mode is set. */
static volatile uint32_t pacing_vbl;
static int pacing_vbl_handler = -1;

static void pacing_vblank(uint32_t code, void *data) {
  (void)code;
  (void)data;
  pacing_vbl++;
  genwait_wake_all((void *)&pacing_vbl);
}

static uint32_t pacing_refresh_now(void) { return pacing_vbl; }

static void pacing_wait_refresh(uint32_t refresh) {
  while ((int32_t)(pacing_vbl - refresh) < 0) {
    genwait_wait((void *)&pacing_vbl, "enj_pacing", 0);
  }
}
#else
/* Host backends have no vblank: the refresh clock is time in 60 Hz periods. */
#define ENJ_PACING_REFRESH_NS 16666667ull

static uint32_t pacing_refresh_now(void) {
  return (uint32_t)(timer_ns_gettime64() / ENJ_PACING_REFRESH_NS);
}

#ifndef __EMSCRIPTEN__
static void pacing_wait_refresh(uint32_t refresh) {
  const uint64_t at_ns = (uint64_t)refresh * ENJ_PACING_REFRESH_NS;
  const uint64_t now_ns = timer_ns_gettime64();
  if ((int64_t)(at_ns - now_ns) > 0) {
    usleep((useconds_t)((at_ns - now_ns) / 1000ull));
  }
}
#endif
#endif

void enj_pacing_set(enj_pacing_t mode) {
#ifdef __DREAMCAST__
  if (mode != ENJ_PACING_FREE && pacing_vbl_handler < 0) {
    pacing_vbl_handler = vblank_handler_add(pacing_vblank, NULL);
  }
#endif
  pacing_mode = mode;
  pacing_tick = 0;
  pacing_restart = 1;
  pacing_drop_next = 0;
}

enj_pacing_t enj_pacing_get(void) { return pacing_mode; }

int enj_pacing_scene_dropped(void) { return pacing_dropped_now; }

uint32_t enj_pacing_tick(void) { return pacing_tick; }

void enj_pacing_stats(enj_pacing_stats_t *out) {
  if (out != NULL) {
    *out = pacing_stats;
  }
}

/* enj_state.c, before the updater: whether this tick shows a scene. */
void enj_pacing_tick_begin(void) {
  pacing_dropped_now = 0;
  if (pacing_mode == ENJ_PACING_FREE) {
    return;
  }
  if (pacing_restart) {
    pacing_due = pacing_refresh_now() + 1;
    pacing_restart = 0;
  }
  pacing_dropped_now =
      pacing_drop_next || (pacing_mode == ENJ_PACING_HALF && (pacing_tick & 1));
  pacing_drop_next = 0;
  if (pacing_dropped_now) {
    enj_render_hold_frame();
  }
}

/* enj_state.c, after the scene: was the tick late, and when is the next one
 * due? Returns 1 with the refresh to wait for, or 0 to start the next tick at
 * once. */
static int pacing_tick_end(uint32_t *wait_for) {
  pacing_stats.ticks++;
  if (pacing_dropped_now) {
    pacing_stats.dropped++;
  } else {
    pacing_stats.rendered++;
  }
  pacing_tick++;
  if (pacing_mode == ENJ_PACING_FREE) {
    return 0;
  }
  const int32_t behind = (int32_t)(pacing_refresh_now() - pacing_due);
  if (behind > 0) {
    pacing_stats.late++;
    if (pacing_mode == ENJ_PACING_DROP_LATE && !pacing_dropped_now &&
        behind <= ENJ_PACING_MAX_BEHIND) {
      /* Keep the schedule: the next tick is already due, and catches up
       * without a scene of its own. */
      pacing_due++;
      pacing_drop_next = 1;
      return 0;
    }
    /* Start the schedule again from the next refresh: the game slows. */
    pacing_due = pacing_refresh_now() + 1;
    return 0;
  }
  *wait_for = pacing_due;
  pacing_due++;
  return 1;
}

#ifdef __EMSCRIPTEN__
/* The browser calls once per animation frame and cannot wait. A callback that
 * comes before the next tick's refresh (a display faster than 60 Hz) runs no
 * tick. */
int enj_pacing_web_due(void) {
  return pacing_mode == ENJ_PACING_FREE || pacing_restart ||
         (int32_t)(pacing_refresh_now() + 1 - pacing_due) >= 0;
}

/* Returns whether a catch-up tick (without a scene) should run in this same
 * callback. */
int enj_pacing_tick_end_web(void) {
  uint32_t wait_for;
  pacing_tick_end(&wait_for);
  return pacing_drop_next;
}
#else
void enj_pacing_tick_end(void) {
  uint32_t wait_for;
  if (pacing_tick_end(&wait_for)) {
    pacing_wait_refresh(wait_for);
  }
}
#endif
