# Getting started

[Documentation](README.md)

enDjinn is used as source: an application directory symlinks its `Makefile`
to enDjinn's `base_link.mk` and builds from there. Dreamcast with KallistiOS
is the reference target and the default.

## Prerequisites

For Dreamcast builds:

- An active KallistiOS environment (`KOS_BASE` set, `kos-cc` on `PATH`).
  enDjinn compiles with `-std=gnu23`; the examples also use C23 `#embed`, so
  the KOS GCC must support both.
- `pvrtex` on `PATH` when the project has textures.
- `dcaconv` on `PATH`, or `SOUNDMACHINE=/path/to/dcaconv`, when the project
  has sound effects. Builds never clone or download it.
- A host `gcc` command (on macOS, Apple's `gcc` wrapper for Clang will do)
  for `tools/font_enj`, which is built on first use when the project has
  TrueType fonts.
- `mkdcdisc` on `PATH` for `.cdi` images, and `sh-elf-objcopy` for `.bin`
  images.

The asset tools are needed by every target, because all three backends
consume the same generated asset files. The extra dependencies of the
development backends are listed in the
[pc-enDjinn](../backends/pc-endjinn/README.md) and
[web-enDjinn](../backends/web-endjinn/README.md) READMEs.

The repository's [`environ.sh`](../environ.sh) is the maintainer's zsh setup
script: it sources `/opt/toolchains/dc/kos/environ.sh` and sets up the
[console queue](console-queue.md). Use your own KOS `environ.sh` if your
toolchain lives elsewhere.

## Build an example

```sh
make -C examples/enj_hello
make -C examples/enj_hello ENJ_TARGET=pc-endjinn
```

The Dreamcast build writes `examples/enj_hello/bin/enj_hello.elf`; the PC
build writes `examples/enj_hello/build/pc-endjinn/enj_hello`. Run the PC
binary from the example directory. `make -C examples` builds every example.

## Create a project

```sh
mkdir -p my-game/code
cd my-game
ln -s /path/to/enDjinn/base_link.mk Makefile
printf 'ENJ_INJECT_QFONT:=1\n' > local.cfg.mk
```

Copy [`enj_hello.c`](../examples/enj_hello/code/enj_hello.c) to `code/` and
run `make`. The output name comes from the directory name, so this builds
`bin/my-game.elf`; `make bin/my-game.cdi` builds a bootable disc image.
`make cfg_info` shows the resolved paths and flags.

The program initialises enDjinn, pushes one mode and runs the frame loop. Its
updater queues a renderer on the punch-through list, which draws text with
the injected quick font. Pressing Start+A+B+X+Y ends it.

## Next steps

- [Using enDjinn in another project](USING_ENDJINN.md) for submodule setup,
  pinning and the integration contract.
- [Modes and the frame loop](modes.md) and [Rendering](rendering.md) for the
  runtime model.
- [Assets](assets.md) to add textures, fonts and sounds.
- The [examples](../examples/README.md), one per feature.
