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
- `timer_ns_gettime64`, `vid_mode`, and fixed 640x480 `vid_set_mode`
- `pvr_init` and `pvr_shutdown`
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
- overlap-aware back-to-front translucent sorting when PVR autosort is enabled,
  with stable average-depth fallback for intersecting or cyclic geometry
- submission-order translucent drawing when PVR autosort is disabled
- OP, PT, then TR Vulkan draw ordering
- caller-owned projected depth, including Dream Driving's road-only decal bias
- textured sprites and polygons, including qfont and enDjinn font rendering
- ARGB1555, RGB565, ARGB4444, YUV422, PAL4, and PAL8 texture decoding
- linear and twiddled texture layouts, VQ decoding, and mip levels
- live ARGB1555, RGB565, ARGB4444, and ARGB8888 palette updates
- nearest and bilinear filtering, texture alpha blending, and punch-through
- 32-bit VRAM-style texture handles matching KOS pointer storage
- modifier-volume headers and 64-byte packets via opaque Vulkan stencil and
  transparent per-fragment event evaluation
- modifier polygon outside/inside colors, UVs, and second texture state

## Modifier-Volume Semantics

Opaque modifier volumes use a depth-aware two-bit Vulkan stencil state machine.
Closed volumes accumulate triangle crossings with XOR/parity, open or planar
volumes can accumulate coverage with OR, and the final polygon folds that
volume into the area result using inclusion or exclusion semantics. The
modifier polygon's inside color, UVs, and optional second texture are then
drawn only at the visible receiver depth.

Translucent modifier triangles are retained in submission order in a
host-visible GPU storage buffer. Each modifier-enabled translucent fragment
evaluates triangle coverage and interpolated inverse depth, reconstructs the
current volume with XOR or OR, and folds completed volumes into its area result
with inclusion or exclusion. Area 0 and area 1 are separate draws with
complementary fragment rejection, so exactly one is blended at each fragment;
overlapping translucent layers can therefore receive different classifications
at the same pixel. PC and Web share a conservative hard limit of 4,096
translucent modifier triangles per frame and reject an overflowing frame
explicitly rather than silently dropping events.

The fragment cost is linear in the submitted triangle count for every
modifier-enabled translucent receiver. Treat 256 triangles as an initial
practical budget, measure the actual scene, and keep the 4,096 limit as a
safety boundary rather than a target. The explicit
`make -C tests modifier-volume-benchmark` workload covers a full-screen
transparent receiver with 0, 12, 48, 256, 1,024, and 4,096 spatially varied
triangles. On an Apple M1 through MoltenVK at a 1280x960 offscreen target, the
September 2026 median measurements were 3.07, 3.07, 3.09, 3.06, 3.10, and
3.08 ms/frame respectively. These numbers establish the local baseline only;
browser, driver, resolution, and receiver coverage can change the result.

This makes modifier classification depth-accurate for the backend's existing
translucent draw order. The overlap-aware dependency sort resolves a global
far-to-near order where one exists and falls back deterministically for
intersecting or cyclic geometry; it is still not tile-accurate.

This is a supported packet subset, not an arbitrary raw PVR command decoder.
Only the render state listed here is decoded from compiled headers.

## Implemented Input

- keyboard-backed port-A `enj_ctrlr_state_t`
- up to four SDL GameControllers mapped to Maple ports A-D, with hot-plug
  reconnect handling
- SDL controller rumble, with Dreamcast Purupuru power and frequency mapped to
  the host controller's low- and high-frequency motors
- browser Gamepad vibration actuators for rumble-capable Web controllers
- Flycast-compatible keyboard bindings: X/C/S/D for A/B/X/Y, F/V for L/R,
  I/J/K/L for the analogue stick, arrows for D-pad, and Enter for Start

Host keyboard shortcuts are translated into the same `cont_state_t` buttons
that a Dreamcast controller would report.

## Accepted But Placeholder Behavior

- `pvr_list_finish` is accepted; list identity is captured by
  `pvr_list_begin`
- `pvr_wait_ready` and `pvr_wait_render_done` are accepted no-ops
- `pvr_fog_table_color` and `pvr_fog_table_linear` do not affect Vulkan output
- `vid_border_color` has no visible host equivalent

## Not Implemented Yet

- PVR culling modes and complete depth, blend, fog, and material-state decoding
- PVR bump-map lighting semantics
- tile-accurate Dreamcast translucent sorting for intersecting/cyclic geometry
- configurable controller mappings and VMU LCD output
- arbitrary PVR packet streams outside the documented colored geometry subset
