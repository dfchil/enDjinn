# enDgine

A lightweight gameplay loop driver for the Sega Dreamcast.

## Overview

enDgine provides a clean, callback-based API for creating games and applications on the Sega Dreamcast using KallistiOS. It handles all the boilerplate of initializing the graphics system, managing the game loop, calculating frame times, and cleaning up resources.

## Features

- ✅ Simple callback-based architecture
- ✅ Automatic frame timing and delta time calculation
- ✅ Built-in FPS limiting and monitoring
- ✅ PowerVR initialization and management
- ✅ Input handling (START button for exit)
- ✅ Minimal dependencies (just KallistiOS)

## Quick Start

```c
#include "endgine.h"

static int my_init(void) {
    // Initialize your game
    return 0;
}

static void my_update(float delta_time) {
    // Update game state
}

static void my_render(void) {
    // Render graphics
}

static void my_cleanup(void) {
    // Clean up resources
}

int main(int argc, char **argv) {
    endgine_config_t config = {
        .init = my_init,
        .update = my_update,
        .render = my_render,
        .cleanup = my_cleanup,
        .target_fps = 60
    };
    
    return endgine_run(&config);
}
```

## Building

Requires KallistiOS (KOS) to be installed and configured:

```bash
export KOS_BASE=/path/to/kos
make
```

This builds:
- `libendgine.a` - The enDgine library
- `endgine-demo.elf` - Example rotating square demo

## Documentation

See [API.md](API.md) for complete API documentation and usage examples.

## License

MIT License - See LICENSE file for details.
