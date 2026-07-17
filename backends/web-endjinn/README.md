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
by the PC backend, including palettized textures, punch-through alpha,
translucent sorting, and the existing 2D modifier-volume stencil behavior.
