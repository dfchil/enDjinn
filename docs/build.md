# Build system

[Documentation](README.md)

An enDjinn project has no build logic of its own. Its `Makefile` is a symlink
to enDjinn's [`base_link.mk`](../base_link.mk), and its settings live in
`local.cfg.mk`.

## Project layout

```text
my-game/
├── Makefile -> vendor/enDJinn/base_link.mk
├── local.cfg.mk             # optional project settings
├── code/                    # every .c file below here is compiled
├── include/                 # optional application headers, on the include path
└── assets/                  # optional source assets
    ├── texture/
    ├── fonts/
    └── sfx/
```

The symlink is intentional: `base_link.mk` resolves the enDjinn directory from
its real path, so the project can live anywhere. It reads enDjinn's
[`cfg.mk`](../cfg.mk) for defaults and then the project's `local.cfg.mk`.
Keep application overrides in `local.cfg.mk`; do not edit the engine's
`cfg.mk`. The project name, `ENJ_BASENAME`, defaults to the directory name.

Every target compiles all of enDjinn's `code/*.c` together with the
application's sources; there is no prebuilt library. The web target also
compiles `.cpp` files below `code/`.

## Targets

`ENJ_TARGET` selects the backend: `dreamcast` (default), `pc-endjinn` or
`web-endjinn`. Pass it on the command line or set it in `local.cfg.mk`.

| Command | Result |
| --- | --- |
| `make` | Dreamcast `bin/<project>.elf` |
| `make bin/<project>.cdi` | Bootable disc image built with `mkdcdisc` from the ELF, `ENJ_ROMDIR` and the boot logo |
| `make bin/<project>.bin` | Raw binary from `sh-elf-objcopy` |
| `make ENJ_TARGET=pc-endjinn` | Native executable `build/pc-endjinn/<project>` |
| `make ENJ_TARGET=web-endjinn` | `build/web-endjinn/<project>.html` with the files Emscripten writes beside it |
| `make assets` | Generated assets only; see [Assets](assets.md) |
| `make cfg_info` | Resolved directories, tools and flags |
| `make auto_variables` | Discovered assets, objects and preprocessor flags |
| `make list`, `make help` | Make targets, short help |

`cfg_info` prints the Dreamcast-oriented settings for every backend; the host
backends' own directories are listed in the
[configuration reference](configuration.md#host-backends).

Dreamcast objects go to `build/`, host objects to `build/pc-endjinn/` or
`build/web-endjinn/`, so the three targets can be built side by side.
Dependency files are generated, so header changes rebuild what uses them.

## Cleaning

`clean` and `mrproper` act on the selected backend:

| Target | `clean` | `mrproper` |
| --- | --- | --- |
| Dreamcast | removes `build/` and everything in `bin/` | also removes the generated assets and `.pal` files, enDjinn's built `font_enj`, and empty directories under `ROMBASEPATH` and `bin/` |
| pc-endjinn | removes `build/pc-endjinn/` | also removes the host output directory and empty directories under `ROMBASEPATH` |
| web-endjinn | removes `build/web-endjinn/` | same as `clean` |

Generated assets are deleted only by the Dreamcast `mrproper`. `mrproper`
removes only enDjinn's own output trees, never anything else in the project
directory.

`examples/Makefile` runs `all`, `clean` or `mrproper` in every example
directory, passing on variables such as `ENJ_TARGET`.

## Toolchains

| Target | Compiler | Notes |
| --- | --- | --- |
| Dreamcast | `kos-cc` with `-std=gnu23` | Needs an active KOS environment; includes `$(KOS_BASE)/Makefile.rules` |
| pc-endjinn | `$(CC)`/`$(CXX)`; C as `-std=gnu23`, backend C++ as `-std=c++23` | See the [pc-enDjinn README](../backends/pc-endjinn/README.md) |
| web-endjinn | `emcc`/`em++` (`EMCC`, `EMXX`) | See the [web-enDjinn README](../backends/web-endjinn/README.md) |

The host targets do not need a KOS environment to compile, but asset
conversion still uses the Dreamcast tools. Every variable is listed in the
[configuration reference](configuration.md).
