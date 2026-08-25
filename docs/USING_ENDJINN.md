# Using enDjinn in another project

This guide describes enDjinn's source-integration contract. Read it alongside
the [hello example](../examples/enj_hello/code/enj_hello.c) and the
[feature examples](../examples/README.md).

## Status and compatibility

enDjinn is an MIT-licensed, Dreamcast-first C23 runtime under active
development. Dreamcast with KallistiOS is the reference target. The
`pc-endjinn` and `web-endjinn`
targets are development backends that run the same application source, but do
not emulate every KOS or PVR feature. Consult the
[PC support matrix](../backends/pc-endjinn/SUPPORTED.md) before relying on a
Dreamcast-specific feature in a portable project.

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
`local.cfg.mk`; do not edit the engine's `cfg.mk`.

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

## Runtime lifecycle

The required order is:

1. Call `enj_state_init_defaults()`.
2. Change fields returned by `enj_state_get()` if the defaults are unsuitable.
3. Call `enj_state_startup()` and handle a non-zero result.
4. Keep at least one caller-owned `enj_mode_t` alive and push it with
   `enj_mode_push()`.
5. Call `enj_state_run()`.

`enj_state_run()` owns the frame loop and shuts down enDjinn subsystems when it
finishes. Request shutdown with `enj_state_flag_shutdown(NULL)`. Mode objects
and their `data` are borrowed, not copied: they must remain alive while present
on the mode stack. `mode_updater` must not be null. A mode name has room for
`ENJ_MODE_NAME_CAPACITY - 1` visible characters plus the terminating null byte.

Index zero is the base mode and cannot be popped. Pushing a mode covers the
current mode. Popping or jumping back reveals an earlier mode and calls its
`on_activation_fn(previous, next)` callback. `enj_mode_set()` replaces the
current pointer without calling activation callbacks; use it only when that is
the intended transition.

## Update and render phases

Each frame, the active mode's updater runs first. The updater changes game
state and queues work with `enj_render_list_add()`. enDjinn later invokes each
renderer in PVR-list order between `pvr_list_begin()` and `pvr_list_finish()`.

The render queue copies the callback and the `data` pointer, not the pointed-to
payload. Consequently:

- `data` must remain valid until the renderer is called later that frame;
- durable mode/game state is the safest payload;
- a pointer to an updater's automatic local variable is unsafe after the
  updater returns; and
- render callbacks must submit commands compatible with the list they were
  queued into.

Invalid render-list values and null callbacks are ignored. The queue allocates
additional fixed-size segments when a list grows; use
`enj_render_print_list_sizes()` in a debug build to inspect its footprint.

## Input

`enj_ctrl_get_states()` returns an enDjinn-owned array with
`MAPLE_PORT_COUNT` entries. A null entry means that no controller is connected
at that port. Non-null entries are refreshed once per frame before the mode
updater runs and remain owned by enDjinn.

Each button is a two-bit state: up, down, pressed this frame, or released this
frame. `enj_ctrlr_button_combo()` is useful for chords that must include a new
press. Abstract controllers let bots or replay streams populate the same
`enj_ctrlr_state_t` representation as physical controllers.

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
sizes after checking their headers.

## Portability checklist

For code intended to run on all three targets:

- use the enDjinn examples and public headers rather than private backend
  symbols;
- check the backend support matrix before using VMU, rumble, or uncommon PVR
  state;
- use paths below `ENJ_CBASEPATH` for packaged read-only assets;
- test both a reference Dreamcast build and the development backend you ship or
  use; and
- do not treat the PC or browser backend as cycle-accurate Dreamcast emulation.

The browser-only custom-pass API lives in `enj_web_render.h` and should be
guarded by the application's web-target compile definition. It is deliberately
not included by the cross-platform aggregate header `enj_enDjinn.h`.

## Updating enDjinn

Before moving a pinned revision, build enDjinn's tests and examples:

```sh
make -C vendor/enDJinn/tests check
make -C vendor/enDJinn/examples ENJ_TARGET=pc-endjinn
```

Then rebuild the application's supported targets from clean object directories.
Public headers are checked independently by the test suite, which catches
missing includes and accidental order dependencies.
