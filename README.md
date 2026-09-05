# enDjinn

enDjinn (a play on “engine” and the invisible helper of myth) is a small,
Dreamcast-first C runtime and build system. It handles recurring KOS/PVR
boilerplate while keeping the Dreamcast rendering model visible: applications
work with modes, PVR render lists, controllers, textures, fonts, sound, and
VMU/rumble devices rather than a generic cross-platform scene API.

It is available under the permissive [MIT License](./LICENSE).

<div>
<img style="height:220px" src="./docs/img/enDjinn.svg" alt="enDjinn logo" />
</div>

## What it provides

- A frame loop that owns PVR scene setup and teardown.
- A stack of `enj_mode_t` game modes, including activation callbacks and soft
  reset support.
- Render-list callbacks for opaque, punch-through, and translucent PVR lists.
- Controller state transitions, abstract controllers, rumble, and VMU hooks.
- Build-time conversion of supported texture, TrueType font, and sound assets.
- A built-in quick font (`qfont`) for fast text rendering, plus generated
  proportional `.enjfont` files.
- `dctrace` and `dcprof` profiling helpers.

The [examples](./examples/README.md) are the best source of working,
feature-specific code.

If you are adopting enDjinn in another repository, start with
[Using enDjinn in another project](./docs/USING_ENDJINN.md). It documents the
current compatibility policy, source layout, lifecycle, callback lifetimes,
resource ownership, and a portability checklist. The concise
[target support matrix](./docs/SUPPORT.md) distinguishes reference,
development, and diagnostic facilities.

## Requirements and project status

enDjinn uses GNU C23 and is integrated as source rather than as a prebuilt
library. Dreamcast/KallistiOS is the reference target. Native and browser
development builds additionally require the dependencies listed in their
respective backend READMEs.

The API is still evolving and there is no versioned stable ABI yet. External
projects should pin a known Git commit (for example with a submodule) and review
public-header changes when updating. Public C headers are independently
includable and provide C linkage when used from C++.

## Quick start: Dreamcast

enDjinn expects an active KallistiOS environment. The repository’s
[`environ.sh`](./environ.sh) is a local convenience script for its configured
toolchain location; use your own KOS `environ.sh` if it lives elsewhere.

Create a project with `code/`, optionally `assets/`, a `local.cfg.mk`, and a
`Makefile` symlink to `base_link.mk`:

```sh
mkdir -p my-game/code
cd my-game
ln -s /path/to/enDjinn/base_link.mk Makefile
printf 'ENJ_INJECT_QFONT:=1\n' > local.cfg.mk
```

Then build from the project directory with `make`. The default Dreamcast
target is `bin/<project-name>.elf`. `make bin/<project-name>.cdi` creates a
CDI image, and `make bin/<project-name>.bin` creates a raw binary.

The minimal application is the [hello example](./examples/enj_hello/code/enj_hello.c):

```c
#include <enDjinn/enj_enDjinn.h>

void render(void *__unused) {
  enj_qfont_write("Hello, enDjinn!", 20, 20, PVR_LIST_PT_POLY);
}

void update(void *__unused) {
  enj_render_list_add(PVR_LIST_PT_POLY, render, NULL);
}

int main(__unused int argc, __unused char **argv) {
  enj_state_init_defaults();
  if (enj_state_startup() != 0) return -1;

  enj_mode_t mode = {.name = "Main", .mode_updater = update};
  enj_mode_push(&mode);
  enj_state_run();
  return 0;
}
```

## Runtime model

Call `enj_state_init_defaults()`, then `enj_state_startup()`, push at least
one mode, and call `enj_state_run()`. A mode’s `mode_updater` is called every
frame. It should update its state and register renderer callbacks with
`enj_render_list_add()`; enDjinn invokes each callback in the matching PVR
phase.

The mode stack is LIFO: push a pause/menu mode over the game, then call
`enj_mode_flag_end_current()` to end it on the next loop iteration and resume
the previous mode. `on_activation_fn` is called when a revealed mode becomes
active. Use `enj_mode_soft_reset_target_set()` to choose where the default
Start+A+B+X+Y soft-reset combination returns.

## Assets and build configuration

By default, source code is read from `code/`, textures from `assets/texture`,
fonts from `assets/fonts`, and sound effects from `assets/sfx`. Generated
Dreamcast assets are placed under `cdrom/<project-name>/` unless overridden.

Texture conversion is selected by the directory below `assets/texture`:
`pal4`, `pal8`, `pal4_vq_tw`, `pal8_vq_tw`, `rgb565_vq_tw`, and
`argb1555_vq_tw` are supported. PAL4 and PAL8 textures are always twiddled by
the hardware format; the `_vq_tw` variants additionally enable VQ compression.
TrueType files in `assets/fonts/<pixel-height>/` become `.enjfont` files. Sound
files in the supported ADPCM or PCM directory layouts become `.dca` files. The
sound converter must be installed as `dcaconv` on `PATH`, or supplied as
`SOUNDMACHINE=/path/to/dcaconv`. Builds never clone dependencies.

Put project-specific overrides in `local.cfg.mk`. The most useful current
variables are:

| Variable | Purpose |
| --- | --- |
| `ENJ_TARGET` | `dreamcast` (default), `pc-endjinn`, or `web-endjinn`. |
| `ENJ_BASENAME` | Output/project name; defaults to the current directory name. |
| `ENJ_CODEDIR` | C source directory; defaults to `./code`. |
| `ENJ_ROMDIR` | CD-ROM asset root; defaults to `cdrom`. |
| `ENJ_BUILDDIR`, `ENJ_BINDIR` | Object and output directories; default to `build` and `bin`. |
| `ENJ_LDLIBS` | Extra libraries passed to the Dreamcast link step. |
| `ENJ_PAL4_PVRTEX_FLAGS`, `ENJ_PAL8_PVRTEX_FLAGS` | Extra `pvrtex` flags for uncompressed palettized textures. |
| `ENJ_PAL4_VQ_TW_PVRTEX_FLAGS`, `ENJ_PAL8_VQ_TW_PVRTEX_FLAGS` | Extra `pvrtex` flags for VQ-compressed palettized textures. |
| `SOUNDMACHINE` | Path to the required `dcaconv` executable. |
| `ENJ_CFLAGS` | Additional C compiler flags. |
| `OPTLEVEL` | Optimisation level passed as `-O`; default is `g`. |
| `ENJ_DEBUG` | Enables `ENJ_DEBUG_PRINT` and Dreamcast debug instrumentation. |
| `ENJ_FSAA` | Enables FSAA and changes `ENJ_XSCALE` accordingly. |
| `ENJ_FRAME_RATE` | Enables a frame-rate deadline when set to a positive value. |
| `ENJ_WEB_SIMD` | Set to `1` to compile and link `web-endjinn` with WebAssembly SIMD128; defaults to `0`. |
| `ENJ_INJECT_QFONT` | Builds and embeds the built-in quick font. |
| `ENJ_ADD_LOGO_TEXTURE` | Adds enDjinn logo textures to the generated assets. |

Set `ENJ_WEB_SIMD=1` to pass `-msimd128` to every WebAssembly compilation and
the final link. Leave it at
the default `0` when producing a scalar compatibility build.

Run `make cfg_info` in a project directory to inspect the resolved build
configuration, and `make auto_variables` to inspect generated asset and
object lists. `make assets` generates the same declared asset blobs for either
selected backend without linking an executable.

## PC backend

Set `ENJ_TARGET=pc-endjinn` to compile the supported runtime subset against
SDL2 and Vulkan/MoltenVK. It recompiles the same application against
KOS-shaped compatibility headers and links PC implementations of the required
KOS symbols; it is not a binary-only replacement for an already compiled
Dreamcast executable. This is a development backend, not a full Dreamcast
emulator.

From an enDjinn application directory:

```sh
make ENJ_TARGET=pc-endjinn
./build/pc-endjinn/my-game
```

The executable name defaults to the application directory name.

The backend supports the geometry, texture and palette formats used by the
examples, PCM sound effects, keyboard and SDL controllers, host save-file
storage, opaque depth/stencil modifier volumes, and depth-aware per-fragment
translucent modifier evaluation. Rumble, VMU LCD output, and some PVR
state remain unsupported. See the
[PC backend README](./backends/pc-endjinn/README.md) for setup and usage and
the [support matrix](./backends/pc-endjinn/SUPPORTED.md) for exact coverage.

## Browser backend

Set `ENJ_TARGET=web-endjinn` to compile the same application and generated
assets to WebAssembly with Emscripten, SDL2, and WebGL 2:

```sh
make ENJ_TARGET=web-endjinn
emrun build/web-endjinn/my-game.html
```

The browser target shares host input, audio, filesystem, and PVR packet
decoding code with the PC backend. The browser owns frame scheduling through
`requestAnimationFrame`. See the
[browser backend README](./backends/web-endjinn/README.md) for details.

## Profiling

Set `ENJ_DCTRACE` or `ENJ_DCPROF` in `local.cfg.mk` to include the corresponding
Dreamcast-side instrumentation. See [Dreamcast profiling](./profilers/README.md)
for capture and analysis commands. Application launch scripts own dcload
addresses, game-specific modes, and output locations.

## Tests

The [test guide](./tests/README.md) lists every routine check and explicit
renderer integration target.

Run the host-side safety, public-header, and backend-shim checks with:

```sh
make -C tests check
```

Run the deterministic pc-enDjinn modifier-volume framebuffer regression with:

```sh
make -C tests modifier-volume-visual
```

This explicit target requires the pc-enDjinn Vulkan dependencies; the ordinary
host checks remain renderer-independent.

Compile the same depth-aware modifier-volume example for WebGL 2 with:

```sh
make -C tests modifier-volume-web
```

Exercise all six modifier toggles in headless Chrome/Chromium with:

```sh
make -C tests modifier-volume-web-visual
```

Set `CHROME=/path/to/chrome` when the browser is not in a conventional install
location. This explicit integration target starts only a temporary localhost
server and browser profile.

Measure the PC translucent-modifier reference workload with:

```sh
make -C tests modifier-volume-benchmark
```

The [support matrix](./backends/pc-endjinn/SUPPORTED.md#modifier-volume-semantics)
records the workload, current baseline, practical budget, and hard limit.

Build every example against the native development backend with:

```sh
make -C examples ENJ_TARGET=pc-endjinn
```
