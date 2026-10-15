# Textures

[Documentation](README.md)

Textures are converted at build time by `pvrtex` into `.dt` files (`DcTx`
format), plus a `.dt.pal` palette (`DPAL` format) for palettised textures.
[Assets](assets.md#textures) lists the source directories and conversion
options. This page covers loading them; declarations are in
[`enj_texture.h`](../include/enDjinn/enj_texture.h).

## Loading

```c
enj_texture_info_t tex;
if (!enj_texture_load_file(ENJ_CBASEPATH "texture/pal8/ship.dt", &tex)) {
  /* handle failure */
}
```

`enj_texture_load_file()` and `enj_texture_load_blob()` validate the `DcTx`
header, allocate PVR memory and upload the texture data. They return 1 on
success and 0 on failure; inspect `tex` only on success. A blob must be a
complete converter-generated file and may be released after the call. The
loaded descriptor records the size, PVR format, the PVR memory pointer `ptr`
and flags for palettised, twiddled, compressed, strided and mipmapped data.

Release the PVR memory with `enj_texture_unload(&tex)`.

## Palettes

A palettised texture needs its palette in PVR palette RAM and its texture
format pointing at that palette. The
[sprite example](../examples/enj_sprite/code/enj_sprite.c) does both:

```c
enj_texture_load_blob(texture_blob, &tex);
enj_texture_bind_palette(&tex, 0);
enj_texture_load_palette_blob(palette_blob, tex.flags.palette_format,
                              tex.flags.palette_position);
```

- `enj_texture_bind_palette(&tex, offset)` sets the palette offset in the
  texture's PVR format word and records it in `tex.flags.palette_position`.
- `enj_texture_load_palette_blob()` and `enj_texture_load_palette_file()`
  write a `DPAL` palette into palette RAM at an entry offset, converting it to
  `PVR_PAL_ARGB8888`, `PVR_PAL_ARGB4444`, `PVR_PAL_RGB565` or
  `PVR_PAL_ARGB1555`. They also set the global palette format, so every
  palette in use must share one format. The palette must fit in the 1024
  entries of palette RAM.

On load, a 16-colour texture gets `PVR_PAL_ARGB4444` in
`flags.palette_format` and other palettised textures get `PVR_PAL_ARGB8888`.
To switch the palette format during a running scene, use
`enj_render_palette_mode_set()` (see [Rendering](rendering.md#frame-hooks)).

## Logo textures

`ENJ_ADD_LOGO_TEXTURE=1` adds enDjinn's own logo to the project's generated
assets as `texture/pal8_vq_tw/enDjinn512.dt` and `enDjinn256.dt`. Several
examples use it.

## Host backends

pc-enDjinn and web-enDjinn decode the same files. Supported pixel formats,
twiddled and VQ layouts, mip levels and live palette updates are listed in
the [pc-enDjinn support document](../backends/pc-endjinn/SUPPORTED.md#implemented-rendering-subset).
