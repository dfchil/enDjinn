# Tests

The default suite is fast and renderer-independent:

```sh
make -C tests check
```

It covers:

- independent inclusion of every public C header;
- public C headers consumed from C++;
- mode-stack, bitmap, and allocation-failure safety;
- texture resource ownership and invalid input;
- host PVR texture and palette decoding;
- translucent dependency ordering;
- Web frame batching, including alpha-cutout batch identity; and
- interleaved-thread `dctrace` decoding.

Explicit integration targets exercise heavier platform seams:

| Target | Coverage | Requirement |
| --- | --- | --- |
| `modifier-volume-visual` | Six PC/Vulkan modifier modes against golden framebuffer fingerprints | SDL2, Vulkan/MoltenVK, Qt Shader Tools |
| `modifier-volume-web` | Compiles the deep modifier example for WebAssembly/WebGL 2 | Emscripten |
| `modifier-volume-web-visual` | Drives all six modes in a real headless browser and checks canvas output | Chrome/Chromium |
| `modifier-volume-benchmark` | Measures transparent receiver cost from 0 through 4,096 modifier triangles | PC backend dependencies |

`modifier-volume-visual-update` rewrites the PC golden data. Use it only for a
reviewed rendering correction, and record why the expected output changed.

The PC backend's `pc-endjinn-kos-abi-contract` target regenerates and validates
the supported KOS ABI subset when a Dreamcast toolchain is active. Example
trees can be compiled independently with `make -C examples ENJ_TARGET=...`.
