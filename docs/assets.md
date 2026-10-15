# Assets

[Documentation](README.md)

enDjinn converts textures, fonts and sound effects at build time. Every file
found under the source directories becomes a generated file under
`ROMBASEPATH`, which is `$(ENJ_ROMDIR)/$(ENJ_BASENAME)` (by default
`cdrom/<project>/`). All three backends use the same generated files; the
backend only changes how they are read at runtime. Application sources are
compiled after the assets are generated.

`make assets` generates them without compiling or linking, and
`make auto_variables` lists what was found.

## Textures

PNG files below `assets/texture/` are converted by `pvrtex`. The first
directory below `assets/texture/` selects the format:

| Directory | Format | Extra flags |
| --- | --- | --- |
| `pal4` | 4-bit palettised, 16 colours | `ENJ_PAL4_PVRTEX_FLAGS` |
| `pal8` | 8-bit palettised, 256 colours | `ENJ_PAL8_PVRTEX_FLAGS` |
| `pal4_vq_tw` | 4-bit palettised, VQ compressed | `ENJ_PAL4_VQ_TW_PVRTEX_FLAGS` |
| `pal8_vq_tw` | 8-bit palettised, VQ compressed | `ENJ_PAL8_VQ_TW_PVRTEX_FLAGS` |
| `rgb565_vq_tw` | RGB565, VQ compressed | — |
| `argb1555_vq_tw` | ARGB1555, VQ compressed | — |

PAL4 and PAL8 textures are always twiddled by the hardware format; the
`_vq_tw` variants additionally enable VQ compression.
`assets/texture/pal8/ship.png` becomes `cdrom/<project>/texture/pal8/ship.dt`
and, because it is palettised, `ship.dt.pal`. The `rgb565_vq_tw` rule also
writes a `.dt.png` preview. A PNG in any other directory has no rule and stops
the build. Loading is described in [Textures](textures.md).

## Fonts

TrueType files below `assets/fonts/<line-height>/` are converted by
[`font_enj`](tools.md#font_enj) using that directory name as the line height
in pixels. `assets/fonts/16/Dina-Regular.ttf` becomes
`cdrom/<project>/fonts/16/Dina-Regular.enjfont`, with a `.png` preview of the
glyph sheet beside it. `font_enj` is built from enDjinn's `tools/font_enj`
the first time it is needed. Loading is described in [Fonts](fonts.md).

## Sound effects

WAV and MP3 files below `assets/sfx/` are converted by `dcaconv`, which must
be on `PATH` or named with `SOUNDMACHINE=/path/to/dcaconv`. The directory
selects the format:

| Directory | Format |
| --- | --- |
| `ADPCM` | 4-bit Yamaha ADPCM |
| `PCM/16` | 16-bit PCM |
| `PCM/8` | 8-bit PCM |

`assets/sfx/PCM/16/hit.wav` becomes `cdrom/<project>/sfx/PCM/16/hit.dca`. A
file in any other directory has no rule and stops the build. The host
backends play only 16-bit PCM; see [Sound](sound.md).

## Reading assets at runtime

`ENJ_CBASEPATH` is a C string macro holding the runtime path of
`ROMBASEPATH`, ending in `/`:

| Target | `ENJ_CBASEPATH` |
| --- | --- |
| Dreamcast | `/cd/<project>/` |
| pc-endjinn | absolute path of `ROMBASEPATH` on the build machine |
| web-endjinn | `/<ROMBASEPATH>/`, preloaded into Emscripten's virtual filesystem |

So `ENJ_CBASEPATH "texture/pal8/ship.dt"` works on every target. A `.cdi`
image places the contents of `ENJ_ROMDIR` at the root of the disc, which is
where `/cd/<project>/` points.

## Embedding assets in the executable

The examples do not read files. They set `ENJ_ROMDIR:=embeds` in
`local.cfg.mk`, so the generated files land in `embeds/<example>/`, and
include them with C23 `#embed`:

```c
static const alignas(32) uint8_t texture_blob[] = {
#embed "../embeds/enj_sprite/texture/pal8_vq_tw/enDjinn512.dt"
};
```

The `_blob` loaders then read from memory. This needs no filesystem on any
target, at the cost of executable size.

## Built-in assets

`ENJ_ADD_LOGO_TEXTURE=1` adds enDjinn's logo as
`texture/pal8_vq_tw/enDjinn512.dt` and `enDjinn256.dt`.
`ENJ_INJECT_QFONT=1` compiles the quick font into the executable (see
[Fonts](fonts.md#the-quick-font)). The disc image's boot logo is
`ENJ_IPLOGO` (default enDjinn's `assets/iplogo.mr`), regenerated with
`makeip` from `ENJ_IPLOGO_SRC` (default `docs/img/MR.image.png`) when the
source is newer.
