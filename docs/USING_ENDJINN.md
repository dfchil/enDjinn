# Using enDjinn in another project

[Documentation](README.md)

This guide describes enDjinn's source-integration contract. Read it alongside
the [hello example](../examples/enj_hello/code/enj_hello.c) and the
[feature examples](../examples/README.md).

## Status and compatibility

enDjinn is an MIT-licensed, Dreamcast-first C23 runtime under active
development. Dreamcast with KallistiOS is the reference target. The
`pc-endjinn` and `web-endjinn`
targets are development backends that run the same application source, but do
not emulate every KOS or PVR feature. Consult the
[target support matrix](./SUPPORT.md) and detailed
[PC backend support document](../backends/pc-endjinn/SUPPORTED.md) before
relying on a Dreamcast-specific feature in a portable project.

There is not yet a versioned, stable ABI. Integrate enDjinn as source, pin a
known commit (a Git submodule is a good fit), and review public-header changes
when updating. Public names use the `enj_` prefix; build variables use
`ENJ_`. Existing APIs are kept source-compatible where practical.

enDjinn's public headers are individually includable from C. They also declare
C linkage when included from C++, so an application does not need to add its
own `extern "C"` wrapper. The runtime itself and the standard examples use C23.

## Project layout

A minimal external project looks like this:

```text
my-game/
├── Makefile -> vendor/enDJinn/base_link.mk
├── local.cfg.mk
├── code/
│   └── main.c
├── include/                 # optional application headers
└── assets/                  # optional source assets
    ├── texture/
    ├── fonts/
    └── sfx/
```

For a submodule-based setup:

```sh
git submodule add <enDjinn-repository-url> vendor/enDJinn
ln -s vendor/enDJinn/base_link.mk Makefile
printf 'ENJ_INJECT_QFONT:=1\n' > local.cfg.mk
make
```

The symlink is intentional: `base_link.mk` resolves the enDjinn directory from
its real path, so the project can live anywhere. Keep application overrides in
`local.cfg.mk`; do not edit the engine's `cfg.mk`. [Build system](build.md)
describes the targets and outputs, and the
[configuration reference](configuration.md) lists every variable.

Useful first commands are:

```sh
make cfg_info
make                         # Dreamcast ELF
make bin/my-game.cdi         # bootable image
make ENJ_TARGET=pc-endjinn   # native development build
make ENJ_TARGET=web-endjinn  # browser build
```

The exact native and browser prerequisites are documented in the
[PC backend README](../backends/pc-endjinn/README.md) and
[web backend README](../backends/web-endjinn/README.md).

## Runtime contract

The runtime rules an application must follow are described in the topic
guides; in short:

- Call `enj_state_init_defaults()`, adjust `enj_state_get()` if needed, call
  `enj_state_startup()`, push a mode and call `enj_state_run()`. See
  [Modes and the frame loop](modes.md).
- Mode objects and their `data` are borrowed while they are on the stack.
- Renderers queued with `enj_render_list_add()` run later in the same frame;
  their `data` must still be valid then. See [Rendering](rendering.md).
- Controller states from `enj_ctrl_get_states()` are owned by enDjinn and
  refreshed once per frame. See [Input](input.md).

## Resource ownership

The caller owns resource descriptors; enDjinn owns the backing allocation after
a successful load:

| Resource | Load/create | Release |
| --- | --- | --- |
| Bitmap | `enj_bitmap_create()` | `enj_bitmap_destroy()` |
| Texture | `enj_texture_load_file()` / `enj_texture_load_blob()` | `enj_texture_unload()` |
| Font | `enj_font_from_file()` / `enj_font_from_blob()` | `enj_font_unload()` |
| Sound | `enj_sound_dca_load_file()` / `enj_sound_dca_load_blob()` | `enj_sound_unload()` |
| Injected qfont | initialized by `enj_state_startup()` | released by the state loop |

Blob-loading functions expect a complete, converter-generated blob to remain
readable for the duration of the call. They copy or upload the data they need;
the caller may release the source blob after a successful return. Load
functions return zero (or `SFXHND_INVALID`) on failure and leave no allocation
for the caller to release.

Texture and palette files use enDjinn's generated `DcTx` and `DPAL` formats;
fonts use `.enjfont`; sounds use `.dca`. Prefer the build rules over hand-made
binary data, because the loaders intentionally assume converter-valid payload
sizes after checking their headers. [Assets](assets.md) describes the build
rules; [Textures](textures.md), [Fonts](fonts.md) and [Sound](sound.md) the
loaders.

## Portability checklist

For code intended to run on all three targets:

- use the enDjinn examples and public headers rather than private backend
  symbols;
- check the backend support matrix before using VMU, rumble, or uncommon PVR
  state;
- use paths below `ENJ_CBASEPATH` for packaged read-only assets;
- use 16-bit PCM for sound effects the host backends must play;
- test both a reference Dreamcast build and the development backend you ship or
  use; and
- do not treat the PC or browser backend as cycle-accurate Dreamcast emulation.

## Updating enDjinn

Before moving a pinned revision, build enDjinn's tests and examples:

```sh
make -C vendor/enDJinn/tests check
make -C vendor/enDJinn/examples ENJ_TARGET=pc-endjinn
```

Then rebuild the application's supported targets from clean object directories.
Public headers are checked independently by the test suite, which catches
missing includes and accidental order dependencies.
