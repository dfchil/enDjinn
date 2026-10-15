# Fonts

[Documentation](README.md)

enDjinn draws text as PVR sprites, one per glyph, from a 4-bit glyph sheet.
There are two sources of fonts: `.enjfont` files generated from TrueType
fonts, and the built-in quick font. Declarations are in
[`enj_font.h`](../include/enDjinn/enj_font.h) and
[`enj_qfont.h`](../include/enDjinn/enj_qfont.h); the
[fonts example](../examples/enj_fonts/code/enj_fonts.c) uses both.

## The quick font

Set `ENJ_INJECT_QFONT=1` in `local.cfg.mk`. The build then compiles in the
committed glyph sheet
[`enj_qfont_data.h`](../include/enDjinn/embeds/enj_qfont_data.h), generated
from Dina at 16 pixels, and `enj_state_startup()` uploads it as an ARGB1555
texture. No asset files are needed.

`enj_qfont_write(text, x, y, list)` draws a string and returns its width in
pixels. Call it from a renderer while that list is open; it recompiles its
sprite header when the list changes. `PVR_LIST_PT_POLY` is recommended.
`enj_qfont_color_set(r, g, b)` sets the sprite colour, and
`enj_qfont_get_header()` and `enj_qfont_get_sprite_hdr()` give access to the
font and its header for use with the general font functions below.

## Generated fonts

TrueType files under `assets/fonts/<line-height>/` become `.enjfont` files at
build time; see [Assets](assets.md#fonts). Load one with
`enj_font_from_file()` or `enj_font_from_blob()`, which return 1 on success
and upload the glyph sheet to PVR memory as a 4-bit texture. Release it with
`enj_font_unload()`.

A loaded font needs a sprite header and 16 palette entries. Each of these
writes the palette at `palette_entry` and compiles a header for one list:

| Function | List | Palette |
| --- | --- | --- |
| `enj_font_PAL_TR_header()` | translucent | white with 16 alpha levels |
| `enj_font_PAL_OP_header()` | opaque | 16 steps from the back colour to the front colour |
| `enj_font_PAL_PT_header()` | punch-through | as opaque, with entry 0 fully transparent |

With `PVR_PAL_ARGB4444` (4-bit palettes) 64 palette slots are available, with
`PVR_PAL_ARGB8888` four. `enj_font_PAL_TR_header()` also sets the global
palette format with `pvr_set_pal_format()`; the other two leave it as it is.

## Drawing text

- `enj_font_string_render(text, font, x, y, hdr)` commits `hdr` (when not
  null), draws the string and returns its width in pixels.
- `enj_font_render_glyph()` draws one glyph after a header has been committed.
- `enj_font_string_width()` measures a string without drawing it.
- `enj_font_scale_set()` sets an integer scale, `enj_font_letter_spacing_set()`
  adds pixels between glyphs and `enj_font_zvalue_set()` sets the depth. These
  are global and apply to the quick font too.

Glyphs cover the printable ASCII range `!` to `~`. A space, or a glyph the
font lacks, advances by a quarter of the scaled line height. The glyph
functions multiply x by `ENJ_XSCALE` themselves.
