# pc-enDjinn

pc-enDjinn is the host backend for compiling enDjinn programs on PC while
keeping the Dreamcast/KOS path as the reference target.

pc-enDjinn is a source-compatible KOS shim for enDjinn applications. A PC
build recompiles the same application and shared runtime code against
KOS-shaped compatibility headers, then links PC implementations of the KOS
symbols instead of the Dreamcast KOS libraries. Dreamcast/KOS remains the ABI
and behavioral reference.

The backend is intentionally narrower than KOS today. It provides the
KOS-shaped platform surface used by enDjinn's examples, including textured and
palettized rendering, and leaves unsupported systems explicitly listed in
`SUPPORTED.md`.

## Quick Start

Install a C/C++ compiler, SDL2, Vulkan, and Qt Shader Tools (`qsb`). On macOS,
the usual Homebrew dependencies are:

```sh
brew install sdl2 molten-vk qt
```

Create an enDjinn application in the normal way, with a `code/` directory and
a `Makefile` symlink to `base_link.mk`:

```sh
mkdir -p my-game/code
cd my-game
ln -s /path/to/enDjinn/base_link.mk Makefile
```

Add the application source and any project overrides in `local.cfg.mk`, then
build and run:

```sh
make ENJ_TARGET=pc-endjinn
./build/pc-endjinn/my-game
```

Run the executable from the application directory so it can find the shaders
under `build/pc-endjinn/`. If `ENJ_BASENAME` is overridden, use that name
instead of `my-game`. Clean the host build with:

```sh
make ENJ_TARGET=pc-endjinn clean
```

An ordinary PC build does not require an active KOS environment. Asset
generation still requires the corresponding Dreamcast conversion tools.
Projects with host-side library dependencies can add their include paths to
`ENJ_HOST_CPPFLAGS` and link objects or static libraries through
`ENJ_HOST_EXTRA_OBJS` in `local.cfg.mk`.

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
- `pc_endjinn_input.cpp`: SDL implementations of Maple controllers and PCM
  sound, plus the current rumble and VMU LCD stubs.
- `pc_endjinn_fs.cpp`: host filesystem and `/vmu/` save-path handling.
- `SUPPORTED.md`: Coverage matrix for implemented, stubbed, and unsupported
  KOS/enDjinn behavior.

## Current Integration Model

Applications include normal enDjinn headers. A PC build sets
`ENJ_TARGET=pc-endjinn`, which selects this backend's compiler/include/link
configuration and its KOS-symbol implementations. This is a backend swap at
build time, not a binary-only replacement for an already compiled Dreamcast
executable.

`enj_state_startup()` still remains the app entry point. On pc-enDjinn, its
normal `pvr_init()` call creates the SDL window and Vulkan instance/surface/device
owned by the backend.

For replay-raster diagnostics only, `ENJ_OFFSCREEN_CAPTURE=1` retains that
same PVR packet collection and Vulkan graphics pipeline but replaces the
swapchain color attachment with a 1280x960 offscreen image.  It deliberately
does not acquire a drawable or present a frame, and creates the required SDL
Vulkan surface window hidden, avoiding macOS GUI-session blocking or a visible
test window while an explicit `ENJ_SCREENSHOT_PATH` readback is requested.  It
is not a separate renderer and is not enabled for normal interactive runs.

Dreamcast builds resolve normal KOS includes to the real toolchain headers.
PC builds prepend `backends/pc-endjinn/include`, resolving those same includes
to the compatibility declarations supplied by this backend.

The host branch builds the same shared enDjinn C sources as the Dreamcast
branch, plus the pc-enDjinn platform backend. Unsupported host behavior remains
explicit no-op shim code.

The examples compile with the same command, for example:

```sh
make -C examples/enj_sprite ENJ_TARGET=pc-endjinn
```

Asset-producing examples still require the normal Dreamcast asset tools.
In particular, sound builds require `dcaconv` on `PATH` or an explicit
`SOUNDMACHINE=/path/to/dcaconv`.

## Host Dependencies

On macOS, the backend discovers SDL2 with `sdl2-config`, uses MoltenVK from
Homebrew's `opt` prefix, and links the required Apple frameworks. On Linux,
SDL2 and the native Vulkan loader are discovered with `pkg-config`. `QSB`,
`SDL2_CONFIG`, `PKG_CONFIG`, `SDL_CFLAGS`, `SDL_LIBS`, `VULKAN_CFLAGS`, and
`VULKAN_LIBS` remain overridable.

Typical Debian/Ubuntu package requirements are SDL2 development headers, the
Vulkan loader and headers, Vulkan drivers for the installed GPU, and Qt's
Shader Tools package providing `qsb`.

On macOS, `HOMEBREW_PREFIX` defaults to `/opt/homebrew`; override it for
another Homebrew location. `QSB`, `SDL2_CONFIG`, `PKG_CONFIG`, `SDL_CFLAGS`,
`SDL_LIBS`, `VULKAN_CFLAGS`, and `VULKAN_LIBS` can also be supplied to `make`.

## Runtime Controls

- X/C/S/D: Dreamcast A/B/X/Y
- F/V: left/right trigger
- I/J/K/L: analogue stick
- Arrow keys: D-pad
- Enter: Start
- Escape: quit
- F11 or Alt+Enter: toggle fullscreen

Up to four SDL GameControllers are assigned to Maple ports A-D in connection
order. They use SDL's standard buttons, D-pad, left stick, and triggers.

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
