# pc-enDjinn

pc-enDjinn is the host backend for compiling enDjinn programs on PC while
keeping the Dreamcast/KOS path as the reference target.

The backend is intentionally narrow today. It provides the KOS-shaped platform
surface currently needed by Dream Driving and leaves unsupported systems
explicitly listed in `SUPPORTED.md`.

## Backend Files

- `backend.mk`: Make variables for SDL2, Vulkan/MoltenVK, compile flags, link
  flags, backend source paths, and backend-owned shader compilation.
- `shaders/`: renderer shaders compiled automatically for every pc-enDjinn
  application.
- `include/kos.h`: small KOS umbrella matching normal application includes.
- `include/arch/` and `include/dc/`: domain-owned compatibility declarations
  for timers, PVR, Maple, video, sound, and related KOS systems.
- `include/pc_endjinn/`: backend support types and the generated ABI contract.
- `enj_platform_pc_endjinn.cpp`: thin KOS symbol adapter used by enDjinn.
- `pc_endjinn_pvr.cpp`: PVR direct-render packet decoding, render lists, and
  renderer-neutral primitive queues.
- `pc_endjinn_vulkan.cpp`: SDL window management, Vulkan resources, pipelines,
  frame construction, and presentation.
- `pc_endjinn_input.cpp`: SDL implementations of Maple, sound, rumble, and
  other currently required KOS symbols.
- `SUPPORTED.md`: Coverage matrix for implemented, stubbed, and unsupported
  KOS/enDjinn behavior.

## Current Integration Model

Applications include normal enDjinn headers. A PC build sets
`ENJ_TARGET=pc-endjinn`, which includes this backend's `backend.mk` and links
the backend objects instead of KOS.

`enj_state_startup()` still remains the app entry point. On pc-enDjinn, its
normal `pvr_init()` call creates the SDL window and Vulkan instance/surface/device
owned by the backend.

Dreamcast builds resolve normal KOS includes to the real toolchain headers.
PC builds prepend `backends/pc-endjinn/include`, resolving those same includes
to the compatibility declarations supplied by this backend.

For a generic host build, use the normal `integrations/enDJinn/base_link.mk`
entry point with `ENJ_TARGET=pc-endjinn`. The host branch builds only the
currently supported core: state, modes, render lists, keyboard controller
mapping, draw helpers, no-op rumble/sound, and the pc-enDjinn platform backend.

## Host Dependencies

On macOS, the backend uses SDL2 and MoltenVK from Homebrew's `opt` prefixes and
links the required Apple frameworks. On Linux, SDL2 and the native Vulkan
loader are discovered with `pkg-config`. `QSB`, `PKG_CONFIG`, `SDL_CFLAGS`,
`SDL_LIBS`, `VULKAN_CFLAGS`, and `VULKAN_LIBS` remain overridable.

Typical Debian/Ubuntu package requirements are SDL2 development headers, the
Vulkan loader and headers, Vulkan drivers for the installed GPU, and Qt's
Shader Tools package providing `qsb`.

## KOS ABI Contract

`include/pc_endjinn/kos_abi_contract.generated.h` records authoritative KOS
constants, structure sizes, alignments, and offsets. The generator extracts
them from assembly produced by the Dreamcast compiler, so the probe itself
does not need to run. `kos_abi_compat.cpp` validates the PC replicas with
`static_assert` during every PC build.

After updating KOS, activate its environment and regenerate the contract from
an enDjinn game directory:

```sh
source /opt/toolchains/dc/kos/environ.sh
make ENJ_TARGET=pc-endjinn pc-endjinn-kos-abi-contract
```

The generated header is committed. Ordinary PC builds consume it without
requiring an active Dreamcast toolchain.
