# Dreamcast

[Documentation](README.md)

Dreamcast with KallistiOS is enDjinn's reference target and the default
`ENJ_TARGET`. Behaviour on the development backends is measured against it.

## Building

Activate a KOS environment so that `KOS_BASE` is set and `kos-cc` is on
`PATH`, then run `make` in the project directory. The build includes
`$(KOS_BASE)/Makefile.rules` and compiles enDjinn and the application with
`kos-cc -std=gnu23`.

| Output | Command | Tool |
| --- | --- | --- |
| `bin/<project>.elf` | `make` | `kos-cc` |
| `bin/<project>.cdi` | `make bin/<project>.cdi` | `mkdcdisc` |
| `bin/<project>.bin` | `make bin/<project>.bin` | `sh-elf-objcopy` |

The `.cdi` image contains the ELF, the contents of `ENJ_ROMDIR` (by default
`cdrom/`) at the root of the disc, and the boot logo `ENJ_IPLOGO`. It uses
the project name as the disc name, today's date as the release date and the
serial `ENJ01`.

## Files at runtime

On Dreamcast, `ENJ_CBASEPATH` is `/cd/<project>/`, the location of the
generated assets on the disc. An ELF loaded over the network sees `/cd/`
only when a disc with those files is in the drive, or when the loader
redirects CD reads (`kos-tool -i <iso>`). Otherwise, embed the assets, as the
examples do (see [Assets](assets.md#embedding-assets-in-the-executable)), or
set `ENJ_CBASEPATH` to another path, for example below the loader's `/pc/`
mapping (`kos-tool -m <path>`).

## Running on hardware

The maintainer's setup loads ELFs over the network with `kos-tool` and
dc-load-ip. [Console queue](console-queue.md) describes how `environ.sh` lets
several shells and agents share one console. The profilers write their output
through dcload's `/pc` mount; see [Dreamcast profiling](../profilers/README.md).

## Development backends

pc-endjinn and web-endjinn compile against KOS-shaped headers in
`backends/pc-endjinn/include`. Their structure sizes and constants are checked
against a contract generated from the real KOS headers; after updating KOS,
regenerate it as described in the
[pc-enDjinn README](../backends/pc-endjinn/README.md#kos-abi-contract).
