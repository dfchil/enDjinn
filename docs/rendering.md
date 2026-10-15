# Rendering

[Documentation](README.md)

enDjinn does not wrap the PVR. It decides when each list is open and calls
application code at that moment; the application still writes PVR headers and
vertices itself, usually through KOS direct rendering (`pvr_dr_target()` /
`pvr_dr_commit()`). Declarations are in
[`enj_render.h`](../include/enDjinn/enj_render.h) and
[`enj_draw.h`](../include/enDjinn/enj_draw.h).

## Render lists

Each frame, the active mode's updater runs first. It changes game state and
queues work with `enj_render_list_add(list, renderer, data)`. enDjinn then
opens each PVR list in enum order — `PVR_LIST_OP_POLY`, `PVR_LIST_OP_MOD`,
`PVR_LIST_TR_POLY`, `PVR_LIST_TR_MOD`, `PVR_LIST_PT_POLY` — and calls that
list's renderers in the order they were queued.

The render queue copies the callback and the `data` pointer, not the
pointed-to payload. Consequently:

- `data` must remain valid until the renderer is called later that frame;
- durable mode/game state is the safest payload;
- a pointer to an updater's automatic local variable is unsafe after the
  updater returns; and
- render callbacks must submit commands compatible with the list they were
  queued into.

Invalid list values and null callbacks are ignored. The queue grows in
segments of 64 entries per list and keeps them for later frames; in a build
with `ENJ_DBG_PRINT`, `enj_render_print_list_sizes()` prints its footprint.

A renderer may be queued on several lists, as the
[fonts example](../examples/enj_fonts/code/enj_fonts.c) does for its opaque,
translucent and punch-through text.

## Frame hooks

- `enj_render_post_callback_set(fn, data)` registers one callback that runs
  after `pvr_scene_finish()` every frame.
- `enj_render_palette_mode_set(fmt)` changes the PVR palette format once the
  current frame's lists are submitted. enDjinn first waits for the PVR to
  finish rendering (`pvr_wait_render_done()`), then calls
  `pvr_set_pal_format()`.

## Sprites and 64-byte primitives

`enj_draw_sprite(corners, hdr, uvs)` submits one textured sprite from four
`{x, y, z}` corners. It commits `hdr` first when it is not null, so several
sprites can share one header committed by the caller. When `uvs` is null the
whole texture is mapped. The sprite, controls and modes examples use it.

Sprites and modifier-volume triangles are 64-byte primitives that are written
through the 32-byte store queue in two halves.
`enj_draw_pvr_dr64_init(&first, &second)` points `first` at the next store
queue slot. Fill `first`, call `enj_draw_pvr_dr64_commit_1st()`, which points
`second` at the following slot, then fill `second` and call
`enj_draw_pvr_dr64_commit_2nd()`. The
[flat modifiers example](../examples/enj_flat_modifiers/code/enj_flat_modifiers.c)
submits modifier triangles this way.

## FSAA and screen coordinates

`ENJ_FSAA=1` enables horizontal FSAA. The PVR then works on a 1280x480
surface, and `ENJ_XSCALE` (from `enj_defs.h`) becomes 2; multiply screen x
coordinates by it, as the examples do.

## Modifier volumes

The two modifier examples are the reference: `enj_flat_modifiers` uses a
planar screen-oriented volume and `enj_deep_modifiers` a closed, clipped cube
over opaque and translucent receivers. `flat` and `deep` are enDjinn teaching
terms, not KOS APIs. The PC and Web backends emulate modifier volumes with
the semantics and limits described in the
[pc-enDjinn support document](../backends/pc-endjinn/SUPPORTED.md#modifier-volume-semantics).
