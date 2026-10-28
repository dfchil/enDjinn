# Modes and the frame loop

[Documentation](README.md)

An application hands control to enDjinn once. From then on, enDjinn calls the
updater of the current mode once per frame. Declarations are in
[`enj_state.h`](../include/enDjinn/enj_state.h) and
[`enj_mode.h`](../include/enDjinn/enj_mode.h).

## Startup and shutdown

The required order is:

1. Call `enj_state_init_defaults()`.
2. Change fields returned by `enj_state_get()` if the defaults are unsuitable.
3. Call `enj_state_startup()` and handle a non-zero result.
4. Keep at least one caller-owned `enj_mode_t` alive and push it with
   `enj_mode_push()`.
5. Call `enj_state_run()`.

The defaults are a 640x480 `PM_RGB888P` display, a black background, PVR
autosort enabled, `PVR_BINSIZE_16` for every list, DMA disabled and a 1 MiB
vertex buffer; FSAA follows `ENJ_FSAA`.
`enj_state_startup()` sets the video mode, initialises the PVR (on the host
backends this creates the window and renderer), initialises the injected
quick font when `ENJ_INJECT_QFONT` is set, and starts controllers, rumble and
the sound system.

`enj_state_run()` owns the frame loop. It returns after shutdown and has
already shut down the PVR, the quick font and the profilers by then. Request
shutdown with `enj_state_flag_shutdown(NULL)`; its unused parameter lets it be
used directly as a mode updater. On shutdown enDjinn clears any VMU screens
and stops rumble motors. A `RELEASEBUILD` makes the Dreamcast reboot on exit
instead of returning to the loader.

## Each frame

1. If the current mode was flagged to end, it is popped.
2. Controller states are refreshed.
3. The soft-reset combination is checked (see below).
4. The current mode's `mode_updater(data)` runs. Under a refresh-locked
   [frame pacing](#frame-pacing), the scene of this tick may be dropped.
5. enDjinn waits for the PVR, then calls the queued renderers list by list
   between `pvr_list_begin()` and `pvr_list_finish()`. See
   [Rendering](rendering.md).
6. The scene is finished, the optional post-render callback runs, and pending
   rumble commands are sent.

## The mode stack

Modes are a LIFO stack of up to `ENJ_MODE_STACK_SIZE` (16) entries. Mode
objects and their `data` are borrowed, not copied: they must remain alive
while present on the stack. `mode_updater` must not be null. A mode name has
room for `ENJ_MODE_NAME_CAPACITY - 1` visible characters plus the terminating
null byte; it is used only for diagnostics.

- `enj_mode_push()` covers the current mode. It returns 0 for a null mode or
  updater, or a full stack. It does not call any activation callback.
- `enj_mode_flag_end_current()` pops the current mode at the start of the next
  frame. Ending the base mode ends the program.
- `enj_mode_pop()` pops immediately. Index zero is the base mode and cannot be
  popped.
- `enj_mode_goto_index()` discards every mode above an index.
- `enj_mode_set()` replaces the current pointer without calling activation
  callbacks; use it only when that is the intended transition.

Popping or jumping back reveals an earlier mode and calls its
`on_activation_fn(previous, next)` callback, where `previous` is the mode that
was on top. The [modes example](../examples/enj_modes/code/enj_modes.c) moves
between a main mode, an information mode and short animation modes.

## Soft reset

By default, holding Start+A+B+X+Y on any controller, with at least one of
those buttons newly pressed, leaves the current mode:

- at the base mode, it shuts enDjinn down;
- otherwise, if a target was set with `enj_mode_soft_reset_target_set()`, it
  jumps back to that index;
- otherwise it pops one mode.

`enj_state_soft_reset_set()` changes the button pattern; its header comment
shows how to build one. A mode with `no_soft_reset` set ignores the
combination, and clearing `enj_state_get()->flags.soft_reset_enabled`
disables it everywhere.

## Frame pacing

By default (`ENJ_PACING_FREE`) the Dreamcast loop runs as fast as the PVR
allows, normally at the display rate. Set `ENJ_FRAME_RATE` to a positive value
to add a frame deadline: the Dreamcast and PC loops sleep until the next
deadline, and the browser skips `requestAnimationFrame` callbacks that arrive
early. Without it, the browser runs one frame per `requestAnimationFrame`
callback.

`enj_pacing_set()` ([`enj_pacing.h`](../include/enDjinn/enj_pacing.h)) locks the
loop to the display refresh instead, one tick (updater plus scene) per refresh,
and decides what a late frame costs. It can be changed at any time, for
example from an options menu, and replaces the `ENJ_FRAME_RATE` deadline while
it is not `ENJ_PACING_FREE`:

| Pacing | A frame is late | Shown as |
| --- | --- | --- |
| `ENJ_PACING_STRICT` | every later tick is delayed: the game slows | 60 fps with slowdown |
| `ENJ_PACING_DROP_LATE` | the next tick runs its updater but shows no new scene, and the schedule keeps its place: game time keeps up | 60 fps that drops frames |
| `ENJ_PACING_HALF` | as `ENJ_PACING_STRICT` | a new scene on every second tick: 30 fps |

A dropped scene is a hold (`enj_render_hold_frame()`): the updater still runs,
so game state must advance in the updater and never in a renderer.
`enj_pacing_scene_dropped()` tells the updater that the scene it is about to
queue will not be shown, so it can skip building it and keep per-scene
bookkeeping (interpolation, once-per-picture effects) in step. DROP_LATE drops
at most every second scene; a stall of more than three refreshes, such as a
load, starts the schedule again instead of dropping. `enj_pacing_stats()`
counts ticks, rendered and dropped scenes and late ticks.

On Dreamcast the refresh is counted by a vblank handler that the first
refresh-locked `enj_pacing_set()` installs. PC and the browser count 60 Hz
periods of the host clock; the browser runs a dropped tick in the same
`requestAnimationFrame` callback as the late one. A game that paces its own
loop (built with `ENJ_FRAME_RATE=0` and waiting on vblank itself) stays with
`ENJ_PACING_FREE` and calls `enj_render_hold_frame()` for the refreshes it
chooses to drop.

`ENJ_SHOWFRAMETIMES=1` colours the Dreamcast video border by phase (update,
waiting for the PVR, rendering) so frame timing is visible on screen.
