# pc-enDjinn Backend Support

pc-enDjinn compiles the normal enDjinn application entry point for PC while
keeping Dreamcast/KOS as the reference target. The application still calls
`enj_state_startup()`, `enj_state_run()`, and the PVR-shaped rendering API;
the selected backend owns SDL, Vulkan, and host input details.

## Implemented Platform Lifecycle

- SDL window and event initialization from the normal `pvr_init()` call
- Vulkan instance, MoltenVK portability, surface, device, and swapchain setup
- Swapchain recreation after resize or out-of-date presentation
- Color and inverse-Z depth attachments
- 640x480 PVR coordinates (1280x480 with FSAA) scaled to the Vulkan viewport
- `timer_ns_gettime64`, `vid_mode`, `vid_set_mode`, and `vid_border_color`
- `pvr_init`, `pvr_shutdown`, `pvr_wait_ready`, and `pvr_wait_render_done`
- `pvr_scene_begin` and `pvr_scene_finish` frame collection/presentation
- `pvr_set_bg_color` as the Vulkan color clear value
- host `enj_state_startup()` and `enj_state_run()`
- SDL Audio playback for 16-bit mono and stereo PCM sound effects
- host `/vmu/` path redirection for save files

## Implemented Rendering Subset

- Dreamcast-style `pvr_dr_target()` / `pvr_dr_commit()` submission
- projected colored triangle packets terminated by `PVR_CMD_VERTEX_EOL`
- projected colored sprite/quad packets, including the pc-enDjinn fourth-Z
  sideband used by the shared Dream Driving submitter
- ARGB vertex color conversion
- backface culling disabled for 2D packet compatibility
- inverse-Z depth testing with `VK_COMPARE_OP_GREATER`
- opaque (`PVR_LIST_OP_POLY`) depth-tested, depth-writing draws
- punch-through (`PVR_LIST_PT_POLY`) depth-writing draws with alpha cutoff
- translucent (`PVR_LIST_TR_POLY`) alpha blending without depth writes
- stable back-to-front translucent sorting when PVR autosort is enabled
- submission-order translucent drawing when PVR autosort is disabled
- OP, PT, then TR Vulkan draw ordering
- the current Dream Driving road-decal depth bias
- textured sprites and polygons, including qfont and enDjinn font rendering
- ARGB1555, RGB565, ARGB4444, YUV422, PAL4, and PAL8 texture decoding
- linear and twiddled texture layouts, VQ decoding, and mip levels
- live ARGB1555, RGB565, ARGB4444, and ARGB8888 palette updates
- nearest and bilinear filtering, texture alpha blending, and punch-through
- 32-bit VRAM-style texture handles matching KOS pointer storage
- modifier-volume headers and 64-byte packets via a Vulkan stencil mask
- modifier polygon outside/inside colors, UVs, and second texture state

## Modifier-Volume Semantics

`PVR_MODIFIER_INCLUDE_LAST_POLY` and `PVR_MODIFIER_EXCLUDE_LAST_POLY` map to
a screen-space Vulkan stencil mask. This covers the ordinary modifier-mask use
case and applies the modifier polygon's inside color, UVs, and optional second
texture within the mask.

Closed 3D PVR shadow volumes built from `PVR_MODIFIER_OTHER_POLY` sequences do
not yet use PVR's depth-aware stencil winding; they currently act as inclusion
masks. Add that winding path only for a project that depends on shadow-volume
semantics.

This is a supported packet subset, not an arbitrary raw PVR command decoder.
Only the render state listed here is decoded from compiled headers.

## Implemented Input

- keyboard-backed port-A `enj_ctrlr_state_t`
- up to four SDL GameControllers mapped to Maple ports A-D, with hot-plug
  reconnect handling
- Flycast-compatible keyboard bindings: X/C/S/D for A/B/X/Y, F/V for L/R,
  I/J/K/L for the analogue stick, arrows for D-pad, and Enter for Start
- controller D-pad camera selection, X recenter, Y course switch, and Start

Host keyboard shortcuts are translated into the same `cont_state_t` buttons
that a Dreamcast controller would report.

## Accepted But Placeholder Behavior

- `pvr_list_finish` is accepted; list identity is captured by
  `pvr_list_begin`
- `pvr_fog_table_color` and `pvr_fog_table_linear` do not affect Vulkan output
- `enj_rumble_*` reports no rumble device
- `vid_border_color` has no visible host equivalent

## Not Implemented Yet

- PVR culling modes and complete depth, blend, fog, and material-state decoding
- PVR bump-map lighting semantics
- tile-accurate Dreamcast translucent sorting
- configurable controller mappings, VMU LCD output, and controller rumble
- arbitrary PVR packet streams outside the documented colored geometry subset
