# web-enDjinn

web-enDjinn compiles an enDjinn application to WebAssembly with Emscripten.
It reuses the private host-common KOS/PVR packet decoder and presents those
packets with WebGL 2. SDL2 supplies browser input and audio.

Install and activate Emscripten, then build an application with:

```sh
make ENJ_TARGET=web-endjinn
emrun build/web-endjinn/my-game.html
```

The application asset directory is preloaded into Emscripten's virtual
filesystem. The browser drives the engine loop through `requestAnimationFrame`;
`ENJ_FRAME_RATE` caps game updates and defaults to 60 in the browser.

Implemented rendering covers the same projected colored/textured packets used
by the PC backend, including palettized textures, punch-through alpha, and the
shared overlap-aware translucent sorter.

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

`web_endjinn_frame.cpp` owns queue conversion, culling, batching, and modifier
event construction. `web_endjinn_webgl.cpp` owns WebGL resources and submission.

Build the modifier-volume compatibility example explicitly with:

```sh
make -C tests modifier-volume-web
```

`make -C tests modifier-volume-web-visual` additionally runs the six example
toggle combinations in headless Chrome/Chromium and verifies live canvas
output. Set `CHROME` if the browser is installed in a non-standard location.
