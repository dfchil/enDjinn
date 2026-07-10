# pc-enDjinn

pc-enDjinn is the host backend for compiling enDjinn programs on PC while
keeping the Dreamcast/KOS path as the reference target.

The backend is intentionally narrow today. It provides the KOS-shaped platform
surface currently needed by Dream Driving and leaves unsupported systems
explicitly listed in `SUPPORTED.md`.

## Backend Files

- `backend.mk`: Make variables for SDL2, Vulkan/MoltenVK, compile flags, link
  flags, and backend source paths.
- `enj_platform_pc_endjinn.cpp`: Host implementations and stubs for the current
  `enj_platform.h` API surface.
- `SUPPORTED.md`: Coverage matrix for implemented, stubbed, and unsupported
  KOS/enDjinn behavior.

## Current Integration Model

Applications include normal enDjinn headers. A PC build sets
`ENJ_TARGET=pc-endjinn`, which includes this backend's `backend.mk` and links
the backend objects instead of KOS.

`enj_state_startup()` still remains the app entry point. On pc-enDjinn, its
normal `pvr_init()` call creates the SDL window and Vulkan instance/surface/device
owned by the backend.

Dreamcast builds continue to include real KOS headers through
`enDjinn/enj_platform.h`.

For a generic host build, use the normal `integrations/enDJinn/base_link.mk`
entry point with `ENJ_TARGET=pc-endjinn`. The host branch builds only the
currently supported core: state, modes, render lists, keyboard controller
mapping, draw helpers, no-op rumble/sound, and the pc-enDjinn platform backend.
