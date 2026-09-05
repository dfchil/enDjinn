# Cleanup migrations

## PC capture diagnostics

PC-only capture declarations moved out of the KOS-shaped PVR header. Include
`<pc_endjinn/capture.h>` when host tooling needs them.

- Replace `pc_endjinn_pvr_request_current_scene_screenshot(path)` with
  `enj_pc_capture_next_frame(path)` and handle its boolean result.
- Replace `pc_endjinn_pvr_presented_frame_count()` with
  `enj_pc_presented_frame_count()`.

The new capture function copies its path and rejects an invalid path or a
second request while one is pending.

## Web custom rendering

The unused `enj_web_render.h` custom-pass and raw texture-binding API was
removed without a compatibility alias. No enDjinn or Dream Driving consumer
used it. Application-specific WebGL rendering should remain in application
code; a new engine extension should be introduced only when a concrete shared
use case establishes its contract.

## SH4ZAM build integration

The unused `ENJ_USE_SH4ZAM` and `SH4ZAM_DIR` build integration was removed.
Applications that use SH4ZAM should own its source, include paths, objects, and
link flags in `local.cfg.mk`. This keeps the math-library version and target
selection with the application that actually consumes it.

## Profiling launch scripts

The hard-coded `run_dtrace.sh`, `run_dcprof.sh`, and editor-specific
`disasm.sh` helpers were removed. They encoded one application's binary names,
mode selection, dcload address, and local toolchain paths. Keep those workflow
choices in the application; enDjinn still provides both target-side profilers
and the reusable `dctrace.py` decoder.
