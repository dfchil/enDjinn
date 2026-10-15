# Configuration reference

[Documentation](README.md)

Build variables are read from enDjinn's [`cfg.mk`](../cfg.mk), then the
project's `local.cfg.mk`, then the `make` command line, each overriding the
previous. [`defines.mk`](../defines.mk) turns the feature switches into
compiler flags. `make cfg_info` and `make auto_variables` print the result.

## Switches are on when set

The feature switches below are tested with Make's `ifdef`: they are on when
set to any non-empty value, **including `0`**. Leave a switch unset, or set it
to the empty string, to turn it off. The commented `ENJ_INJECT_QFONT:=0` and
`ENJ_ADD_LOGO_TEXTURE:=0` lines in `cfg.mk` would turn the features on if
uncommented. `ENJ_WEB_SIMD` is the exception: it is compared with `1`.

## Project and paths

| Variable | Default | Purpose |
| --- | --- | --- |
| `ENJ_TARGET` | `dreamcast` | `dreamcast`, `pc-endjinn` or `web-endjinn`. |
| `ENJ_BASENAME` | directory name | Project and output name. |
| `ENJ_CODEDIR` | `./code` | Application sources; searched recursively. |
| `ENJ_TEXTURE_SRC_DIR` | `./assets/texture` | Texture sources. |
| `ENJ_FONTS_SRC_DIR` | `./assets/fonts` | TrueType sources. |
| `ENJ_SOUNDFX_SRC_DIR` | `./assets/sfx` | Sound effect sources. |
| `ENJ_ROMDIR` | `cdrom` | Root of the generated assets; the disc image root. |
| `ROMBASEPATH` | `$(ENJ_ROMDIR)/$(ENJ_BASENAME)` | Directory the assets are generated in. |
| `ENJ_CBASEPATH` | per target, see [Assets](assets.md#reading-assets-at-runtime) | Runtime path of `ROMBASEPATH`, defined as a C string. |
| `ENJ_BUILDDIR` | `build` | Dreamcast object directory. |
| `ENJ_BINDIR` | `bin` | Dreamcast output directory. |

## Feature switches

| Variable | Effect |
| --- | --- |
| `ENJ_INJECT_QFONT` | Compiles in and initialises the [quick font](fonts.md#the-quick-font). |
| `ENJ_ADD_LOGO_TEXTURE` | Adds enDjinn's logo textures to the generated assets. |
| `ENJ_FSAA` | Passed as `-DENJ_FSAA=<value>`; `1` enables horizontal FSAA and sets `ENJ_XSCALE` to 2. |
| `ENJ_FRAME_RATE` | Positive value: frame deadline in frames per second. See [frame pacing](modes.md#frame-pacing). |
| `ENJ_SHOWFRAMETIMES` | `1` colours the Dreamcast video border by frame phase. |
| `ENJ_DBG_PRINT` | Enables `ENJ_DEBUG_PRINT` output on stdout. On Dreamcast it also counts cache misses and prints them at shutdown, appending them to `ENJ_CBASEPATH "/pstats.txt"`. |
| `ENJ_DBG_GDB` | Calls KOS `gdb_init()` in `enj_state_init_defaults()` (Dreamcast). |
| `ENJ_DCTRACE` | Function-entry tracing; Dreamcast only. See [profiling](../profilers/README.md). |
| `ENJ_DCPROF` | Sampling profiler; Dreamcast only. See [profiling](../profilers/README.md). |
| `RELEASEBUILD` | Defines `RELEASEBUILD`, drops debug information at link time, and makes the Dreamcast reboot on exit. Without it, `-g` is added. |

There is no `ENJ_DEBUG` variable; use `ENJ_DBG_PRINT`.

## Compiling and linking

| Variable | Default | Purpose |
| --- | --- | --- |
| `OPTLEVEL` | `g` (`2` on web-endjinn) | Passed as `-O$(OPTLEVEL)` on every target. On pc-endjinn it comes after the `-O2` in `ENJ_HOST_CFLAGS` and so takes effect. |
| `ENJ_CFLAGS` | `-std=gnu23` | Dreamcast C flags. Append with `+=` to keep the language standard. |
| `ENJ_INCLUDES` | KOS utils, `./include`, enDjinn `include/` | Dreamcast include flags. |
| `ENJ_CC` | `kos-cc` | Dreamcast compiler. |
| `ENJ_LDLIBS` | empty | Extra libraries for the Dreamcast link. |
| `ENJ_LDFLAGS` | empty | Extra link flags on every target. |
| `OBJS` | empty | Extra Dreamcast objects; may be started in `local.cfg.mk`. |

C macros that are not Make variables can be passed in the compiler flags, for
example `ENJ_CFLAGS += -DENJ_MODE_STACK_SIZE=32` (default 16).

## Asset tools

| Variable | Default | Purpose |
| --- | --- | --- |
| `SOUNDMACHINE` | `dcaconv` | Sound converter, on `PATH` or as a path. |
| `ENJ_PAL4_PVRTEX_FLAGS`, `ENJ_PAL8_PVRTEX_FLAGS` | empty | Extra `pvrtex` flags for `pal4` and `pal8` textures. |
| `ENJ_PAL4_VQ_TW_PVRTEX_FLAGS`, `ENJ_PAL8_VQ_TW_PVRTEX_FLAGS` | empty | Extra `pvrtex` flags for `pal4_vq_tw` and `pal8_vq_tw` textures. |
| `ENJ_IPLOGO` | enDjinn `assets/iplogo.mr` | Boot logo for `.cdi` images. |
| `ENJ_IPLOGO_SRC` | enDjinn `docs/img/MR.image.png` | Source of `ENJ_IPLOGO`. |
| `ENJ_MAKEIP` | `$(KOS_BASE)/utils/makeip/makeip` | Tool that builds `ENJ_IPLOGO`. |

## Host backends

pc-endjinn:

| Variable | Default | Purpose |
| --- | --- | --- |
| `ENJ_HOST_BUILD_DIR`, `ENJ_HOST_BINDIR` | `build/pc-endjinn` | Object and output directories. |
| `ENJ_HOST_TARGET` | `$(ENJ_HOST_BINDIR)/$(ENJ_BASENAME)` | Executable path. |
| `ENJ_HOST_CFLAGS`, `ENJ_HOST_CXXFLAGS` | `-std=gnu23 -O2 -g …`, `-std=c++23 -O2 -g …` | C and C++ flags; setting one replaces the default. |
| `ENJ_HOST_CPPFLAGS` | include paths and backend flags | Append (`+=`) include paths for host-side libraries. |
| `ENJ_HOST_APP_CPPFLAGS` | empty | Preprocessor flags for application sources only. |
| `ENJ_HOST_EXTRA_OBJS` | empty | Extra objects or static libraries to link. |

The SDL2, Vulkan and `qsb` variables (`QSB`, `SDL2_CONFIG`, `PKG_CONFIG`,
`SDL_CFLAGS`, `SDL_LIBS`, `VULKAN_CFLAGS`, `VULKAN_LIBS`, `HOMEBREW_PREFIX`,
`MOLTENVK_PREFIX`) are described in the
[pc-enDjinn README](../backends/pc-endjinn/README.md#host-dependencies), as
are the runtime environment variables for capture and diagnostics.

web-endjinn:

| Variable | Default | Purpose |
| --- | --- | --- |
| `ENJ_WEB_BUILD_DIR`, `ENJ_WEB_BINDIR` | `build/web-endjinn` | Object and output directories. |
| `ENJ_WEB_TARGET` | `$(ENJ_WEB_BINDIR)/$(ENJ_BASENAME).html` | Output page. |
| `ENJ_WEB_CFLAGS`, `ENJ_WEB_CXXFLAGS` | `-std=gnu23 -O2 -g …`, `-std=c++23 -O2 -g …` | C and C++ flags; setting one replaces the default. |
| `ENJ_WEB_CPPFLAGS`, `ENJ_WEB_LDFLAGS` | backend flags | Append (`+=`) project flags. |
| `ENJ_WEB_EXTRA_OBJS` | empty | Extra objects to link. |
| `ENJ_WEB_SIMD` | `0` | `1` adds `-msimd128` to every compile and the link. |
| `EMCC`, `EMXX` | `emcc`, `em++` | Emscripten compilers. |

Both host backends accept `ENJ_INTEGER_PRESENT_WIDTH` and
`ENJ_INTEGER_PRESENT_HEIGHT` as preprocessor definitions; see their READMEs.
