# Tools

[Documentation](README.md)

The `tools/` directory holds host programs used by the build and the
console workflow. Converters that enDjinn calls but does not contain —
`pvrtex`, `dcaconv`, `mkdcdisc`, `makeip` — must be installed separately; see
[Getting started](getting-started.md#prerequisites).

## font_enj

[`tools/font_enj`](../tools/font_enj/font_enj.c) converts a TrueType font into
an `.enjfont` glyph sheet with `stb_truetype`. The build compiles it into
`tools/font_enj/bin/font_enj` inside the enDjinn checkout the first time a
project has fonts, and the Dreamcast `mrproper` deletes it again.

The font rule runs it as:

```sh
font_enj -i <font.ttf> -o <out.enjfont> -l <line-height> -p <preview.png>
```

| Option | Meaning |
| --- | --- |
| `-i`, `--input` | TrueType font. |
| `-o`, `--output` | Output `.enjfont`. |
| `-l`, `--lineheight` | Line height in pixels. |
| `-p`, `--png_control` | Also write the glyph sheet as a PNG for inspection. |
| `-x`, `--exclude` | Characters to leave out. |
| `-b`, `--bitmap_palette` | One-bit glyphs instead of 16 levels. Used for the quick font; `enj_font_from_file()` and `enj_font_from_blob()` expect 4-bit sheets. |
| `-h` | Changes how the glyph sheet grows when the glyphs do not fit. |
| `-v`, `--verbose` | Print progress. |

[`fonts.mk`](../fonts.mk) also has a rule that regenerates the quick font
header, `include/enDjinn/embeds/enj_qfont_data.h`, from enDjinn's
`assets/fonts/16/Dina-Regular.ttf` with `-l 16 -x "~" -h -b` and `xxd -n`.
No build target depends on it, so builds use the committed header; make the
header file as an explicit target, with `ENJ_INJECT_QFONT` set, to refresh
it.

## kos-tool wrapper

[`tools/dctool/kos-tool`](../tools/dctool/kos-tool) queues `kos-tool`
clients for one console. It is described in [Console queue](console-queue.md).

## Profilers

[`profilers/`](../profilers/README.md) contains the Dreamcast-side function
tracer and sampling profiler and the `dctrace.py` decoder.

## KOS ABI contract generator

[`backends/pc-endjinn/tools/generate_kos_abi_contract.sh`](../backends/pc-endjinn/tools/generate_kos_abi_contract.sh)
regenerates the KOS structure-layout contract that the host backends check
against. Run it through `make pc-endjinn-kos-abi-contract`; see the
[pc-enDjinn README](../backends/pc-endjinn/README.md#kos-abi-contract).

## Other directories

- `tools/stb/` holds the `stb_truetype.h` and `stb_image_write.h` headers that
  `font_enj` uses.
- `tools/kmeans_enj/` is an unfinished colour-quantisation experiment. Its
  `Makefile` names a `main.c` that does not exist, and nothing in the build
  uses it.
