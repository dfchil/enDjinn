# enDjinn documentation

[Project overview](../README.md)

## Getting started

- [Getting started](getting-started.md) — prerequisites, first build and a new project.
- [Using enDjinn in another project](USING_ENDJINN.md) — source integration, pinning, ownership and portability.
- [Examples](../examples/README.md) — one small program per feature.

## Runtime API

- [Modes and the frame loop](modes.md) — startup, the mode stack, soft reset and frame pacing.
- [Rendering](rendering.md) — render lists, callback lifetimes, sprites and FSAA.
- [Input](input.md) — controller states, button transitions and abstract controllers.
- [Textures](textures.md) — loading textures and palettes.
- [Fonts](fonts.md) — the quick font and generated `.enjfont` fonts.
- [Sound](sound.md) — loading and playing DCA sound effects.
- [Rumble and VMU](rumble-and-vmu.md) — rate-limited rumble and VMU save files.
- C API: [everything](../include/enDjinn/enj_enDjinn.h),
  [state](../include/enDjinn/enj_state.h), [modes](../include/enDjinn/enj_mode.h),
  [render lists](../include/enDjinn/enj_render.h), [drawing](../include/enDjinn/enj_draw.h),
  [controllers](../include/enDjinn/enj_ctrl.h), [rumble](../include/enDjinn/enj_rumble.h),
  [textures](../include/enDjinn/enj_texture.h), [fonts](../include/enDjinn/enj_font.h),
  [quick font](../include/enDjinn/enj_qfont.h), [sound](../include/enDjinn/enj_sound.h),
  [bitmaps](../include/enDjinn/enj_bitmap.h).

## Build system and configuration

- [Build system](build.md) — project layout, `base_link.mk`, make targets and cleaning.
- [Configuration reference](configuration.md) — every `ENJ_*` variable and feature switch.
- [Assets](assets.md) — source directories, converters, runtime paths and embedding.

## Targets and backends

- [Target support](SUPPORT.md) — what each backend implements, at a glance.
- [Dreamcast](dreamcast.md) — the reference target: outputs, disc images and runtime paths.
- [pc-enDjinn](../backends/pc-endjinn/README.md) — SDL2/Vulkan setup, controls, diagnostics and the KOS ABI contract.
- [pc-enDjinn support details](../backends/pc-endjinn/SUPPORTED.md) — exact PVR, input and modifier-volume coverage.
- [web-enDjinn](../backends/web-endjinn/README.md) — Emscripten/WebGL 2 setup and browser behaviour.

## Tools and profiling

- [Tools](tools.md) — `font_enj` and the other `tools/` directories.
- [Console queue](console-queue.md) — `environ.sh` and sharing one Dreamcast between sessions.
- [Dreamcast profiling](../profilers/README.md) — `dctrace` and `dcprof`.

## Contributing and maintenance

- [Contributing](contributing.md) — project boundaries, code rules and checks.
- [Tests](../tests/README.md) — the fast suite and the explicit renderer targets.
- [Agent instructions](../AGENTS.md), with the [issue tracker](agents/issue-tracker.md)
  and [domain documentation](agents/domain.md) conventions.
- [Maintainability cleanup](../.scratch/endjinn-maintainability-cleanup/spec.md) — the
  specification behind the current boundaries, and its
  [migration notes](../.scratch/endjinn-maintainability-cleanup/migrations.md) for removed APIs.

## Not yet documented

- Bitmaps: `enj_bitmap.h` (a 1-bit bitmap with set, clear, line and PNM
  export) is described only in its header.
- Modifier volumes from application code: the two modifier examples and the
  host semantics in the pc-enDjinn support document are all there is.
- File formats: `.enjfont`, `DcTx`, `DPAL` and `.dca` have no written
  specification; the definitions are `enj_font_types.h`, `pvrtex`'s
  `file_dctex.h` and `ext/dca_file.h`.
- Host backend internals: the shared `backends/host-common/` packet decoder
  and translucent sorter are described only by the file list in the
  pc-enDjinn README.
- Deploying web builds: only `emrun` is described; there is no guide to
  serving the output or replacing Emscripten's default page.
- `CONTEXT.md` and `docs/adr/`, which the agent instructions refer to, do not
  exist yet.
- `.vscode/` tasks and scripts are undocumented.
