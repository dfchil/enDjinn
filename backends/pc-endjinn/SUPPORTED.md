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
- `timer_ns_gettime64`, `vid_mode`, `vid_set_mode`, and `vid_border_color`
- `pvr_init`, `pvr_shutdown`, `pvr_wait_ready`, and `pvr_wait_render_done`
- `pvr_scene_begin` and `pvr_scene_finish` frame collection/presentation
- `pvr_set_bg_color` as the Vulkan color clear value
- host `enj_state_startup()` and `enj_state_run()`

## Implemented Rendering Subset

- Dreamcast-style `pvr_dr_target()` / `pvr_dr_commit()` submission
- projected colored triangle packets terminated by `PVR_CMD_VERTEX_EOL`
- projected colored sprite/quad packets, including the pc-enDjinn fourth-Z
  sideband used by the shared Dream Driving submitter
- ARGB vertex color conversion
- clockwise backface culling
- inverse-Z depth testing with `VK_COMPARE_OP_GREATER`
- opaque (`PVR_LIST_OP_POLY`) depth-tested, depth-writing draws
- punch-through (`PVR_LIST_PT_POLY`) depth-writing draws with alpha cutoff
- translucent (`PVR_LIST_TR_POLY`) alpha blending without depth writes
- stable back-to-front translucent sorting when PVR autosort is enabled
- submission-order translucent drawing when PVR autosort is disabled
- OP, PT, then TR Vulkan draw ordering
- the current Dream Driving road-decal depth bias

This is a supported packet subset, not an arbitrary raw PVR command decoder.
Compiled headers currently contribute their flat ARGB color; complete header
render state is not decoded yet.

## Implemented Input

- keyboard-backed port-A `enj_ctrlr_state_t`
- SDL GameController-backed port A with hot-plug reconnect handling
- WASD steering/throttle/brake mapping
- arrow-key D-pad mapping
- left-stick steering, A/right-trigger acceleration, and B/left-trigger braking
- controller D-pad camera selection, X recenter, Y course switch, and Start
- platform-neutral application input actions with held and pressed states
- abstract B, E, M, R, and F1-F4 key sources for current host applications

SDL scancodes remain inside enDjinn's pc-enDjinn input implementation. Games
bind their own action IDs and query `enj_input_action_down()` or
`enj_input_action_pressed()`.

## Accepted But Placeholder Behavior

- `pvr_list_finish` is accepted; list identity is captured by
  `pvr_list_begin`
- `pvr_set_pal_format` stores no active palette behavior
- `pvr_fog_table_color` and `pvr_fog_table_linear` do not affect Vulkan output
- `enj_rumble_*` reports no rumble device
- `enj_sound_*` performs no playback and returns `SFXHND_INVALID` where needed
- `vid_border_color` has no visible host equivalent

## Not Implemented Yet

- complete PVR header state decoding for culling, depth modes, blend factors,
  fog, and material state
- texture upload, texture formats, palettes, and textured sprites/polygons
- qfont/text rendering
- tile-accurate Dreamcast translucent sorting
- multiple simultaneous game controllers and configurable controller mappings
- VMU, rumble, and sound backends
- arbitrary PVR packet streams outside the documented colored geometry subset
