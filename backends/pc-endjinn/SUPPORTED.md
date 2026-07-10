# pc-enDjinn Backend Support

pc-enDjinn is a host backend for compiling enDjinn code on PC while keeping
Dreamcast/KOS as the reference target.

## Implemented

- `timer_ns_gettime64`
- `vid_mode`
- `vid_set_mode`
- `vid_border_color`
- `pvr_init`
- `pvr_shutdown`
- `pvr_wait_ready`
- `pvr_scene_begin`
- `pvr_scene_finish`
- `pvr_list_begin`
- `pvr_list_finish`
- `pvr_wait_render_done`
- `pvr_set_bg_color`
- `pvr_set_pal_format`
- `pvr_fog_table_color`
- `pvr_fog_table_linear`
- `pvr_dr_target`
- `pvr_dr_commit`
- `pc_endjinn_platform_set_video_size`
- host `enj_state_startup`
- host `enj_state_run`
- SDL keyboard to `enj_ctrlr_state_t`
- `enj_rumble_*` no-op API surface
- `enj_sound_*` no-op API surface

## Current Rendering Contract

The current usable render path is projected colored quad submission, with the
app/game code still using the Dreamcast-oriented renderer and the PC backend
consuming the final projected primitive stream.

## Stubbed Or Placeholder Behavior

- PVR scene/list lifecycle functions are currently no-ops.
- Fog, palette format, and background color calls are accepted but not applied.
- `pvr_dr_target` / `pvr_dr_commit` provide a placeholder packet buffer and do
  not decode arbitrary PVR packets.
- Rumble calls report no device.
- Sound calls return `SFXHND_INVALID` / no playback.

## Not Implemented Yet

- Maple device discovery/state beyond keyboard-as-port-A
- VMU
- texture upload / texture formats / palettes
- qfont/text rendering
- arbitrary raw PVR packet decoding
- textured sprites/polys
