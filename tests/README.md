# Tests

[Documentation](../docs/README.md)

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
| `frame-pacing-hw` | Runs each frame pacing under translucent load on a Dreamcast and checks that DROP_LATE keeps real time where STRICT slows and HALF shows every second scene | A shell that sourced `environ.sh`, a loader-ready console at `DCTOOL_HOST` |
| `modifier-volume-benchmark` | Measures transparent receiver cost from 0 through 4,096 modifier triangles | PC backend dependencies |

Run one with, for example, `make -C tests modifier-volume-visual`. The
default suite does not need the renderer dependencies. Set
`CHROME=/path/to/chrome` when the browser is not in a conventional install
location; `modifier-volume-web-visual` starts only a temporary localhost
server and browser profile. The benchmark's workload, current baseline,
practical budget and hard limit are recorded in the
[support matrix](../backends/pc-endjinn/SUPPORTED.md#modifier-volume-semantics).

`modifier-volume-visual-update` rewrites the PC golden data. Use it only for a
reviewed rendering correction, and record why the expected output changed.

The PC backend's `pc-endjinn-kos-abi-contract` target regenerates and validates
the supported KOS ABI subset when a Dreamcast toolchain is active. Build every
example against the native development backend with:

```sh
make -C examples ENJ_TARGET=pc-endjinn
```
