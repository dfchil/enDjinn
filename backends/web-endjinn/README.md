# web-enDjinn

[Documentation](../../docs/README.md)

web-enDjinn compiles an enDjinn application to WebAssembly with Emscripten.
It reuses the private host-common KOS/PVR packet decoder and presents those
packets with WebGL 2. SDL2 supplies browser input and audio. Input, audio,
filesystem and PVR packet decoding are shared with the
[PC backend](../pc-endjinn/README.md), and so are its keyboard bindings.

Install and activate Emscripten, so that `emcc`, `em++` and `emrun` are on
`PATH`, then build an application with:

```sh
make ENJ_TARGET=web-endjinn
emrun build/web-endjinn/my-game.html
```

Asset generation still uses the Dreamcast conversion tools; see
[Assets](../../docs/assets.md).

The application asset directory is preloaded into Emscripten's virtual
filesystem at the same path, which is what `ENJ_CBASEPATH` points to. The
browser drives the engine loop through `requestAnimationFrame`: without
`ENJ_FRAME_RATE` the game updates once per animation frame, and with a
positive `ENJ_FRAME_RATE` callbacks that arrive before the next deadline are
skipped. KOS file calls on `/vmu/` paths go to an IndexedDB-backed directory,
so saves survive a page reload.

Set `ENJ_WEB_SIMD=1` to pass `-msimd128` to every WebAssembly compilation and
the final link. Leave it at the default `0` when producing a scalar
compatibility build. The other `ENJ_WEB_*` variables are listed in the
[configuration reference](../../docs/configuration.md#host-backends).

Implemented rendering covers the same projected colored/textured packets used
by the PC backend, including palettized textures, punch-through alpha, and the
shared overlap-aware translucent sorter. Like the PC backend, it plays only
16-bit PCM sound effects.

Applications may define both `ENJ_INTEGER_PRESENT_WIDTH` and
`ENJ_INTEGER_PRESENT_HEIGHT` in their Web preprocessor flags. The WebGL backend
keeps the canvas backing store synchronized with its CSS size and browser pixel
ratio, then centers the largest whole-number multiple of that aperture on
black. A target smaller than one aperture uses an aspect-preserving fractional
downscale.

Modifier volumes follow the PC backend's depth-aware semantics. Opaque
receivers first establish visible depth, then WebGL's stencil buffer tracks the
current volume and accumulated area result in separate bits. Transparent
receivers upload their modifier triangles to an `RGBA32F` data texture and
evaluate the ordered XOR/OR and include/exclude events independently at each
fragment depth. PC and Web share a 4,096-triangle hard limit, after which the
frame is rejected instead of silently dropping events. The fragment cost is
linear in the triangle count; start with a practical budget of 256 triangles
affecting transparent receivers and measure the target browser. The PC
reference workload and its machine-specific baseline are documented in the
[support matrix](../pc-endjinn/SUPPORTED.md#modifier-volume-semantics).

## Backend files

- `backend.mk`: Emscripten compilers, compile and link flags, and source list.
- `enj_platform_web_endjinn.cpp`: KOS symbol adapter used by enDjinn.
- `web_endjinn_frame.cpp`: queue conversion, culling, batching, and modifier
  event construction.
- `web_endjinn_webgl.cpp`: WebGL resources, submission and fullscreen.
- `web_endjinn_input.cpp`: browser Gamepad API access: gamepad order,
  analogue triggers and rumble through the vibration actuator.
- `web_endjinn.js`: mounts the IndexedDB `/vmu` directory before start-up
  and adds `Module.requestFullscreen()`.

## Tests

Build the modifier-volume compatibility example explicitly with:

```sh
make -C tests modifier-volume-web
```

`make -C tests modifier-volume-web-visual` additionally runs the six example
toggle combinations in headless Chrome/Chromium and verifies live canvas
output. Set `CHROME` if the browser is installed in a non-standard location.
