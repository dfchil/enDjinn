# web-enDjinn

web-enDjinn compiles an enDjinn application to WebAssembly with Emscripten.
It reuses the PC backend's KOS/PVR packet decoder and presents those packets
with WebGL 2. SDL2 supplies browser input and audio.

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

Modifier volumes follow the PC backend's depth-aware semantics. Opaque
receivers first establish visible depth, then WebGL's stencil buffer tracks the
current volume and accumulated area result in separate bits. Transparent
receivers upload their modifier triangles to an `RGBA32F` data texture and
evaluate the ordered XOR/OR and include/exclude events independently at each
fragment depth. The upload is bounded by the browser's reported maximum texture
size and rejects an overflowing frame instead of silently dropping events.

Build the modifier-volume compatibility example explicitly with:

```sh
make -C tests modifier-volume-web
```
