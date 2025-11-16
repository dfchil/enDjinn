# enDgine API Documentation

## Overview

enDgine is a lightweight gameplay loop driver for the Sega Dreamcast. It provides a clean, callback-based API for creating games and applications using KallistiOS.

## Features

- Simple callback-based game loop
- Automatic frame timing and delta time calculation
- Built-in FPS limiting and monitoring
- Automatic input handling for exit (START button)
- PowerVR initialization and management
- Cross-frame state management

## Building

### Prerequisites

1. KallistiOS (KOS) must be installed and configured
2. Set the `KOS_BASE` environment variable to your KOS installation path
3. Ensure the KOS toolchain is in your PATH

### Build Commands

```bash
# Build the library and example
make

# Build just the library
make libendgine.a

# Clean build artifacts
make clean

# Install library (requires DESTDIR)
make install DESTDIR=/path/to/install
```

## API Reference

### Data Types

#### `endgine_config_t`

Configuration structure for the game loop.

```c
typedef struct {
    endgine_init_func init;       /* Called once at startup */
    endgine_update_func update;   /* Called every frame with delta time */
    endgine_render_func render;   /* Called every frame for rendering */
    endgine_cleanup_func cleanup; /* Called once at shutdown */
    uint32_t target_fps;          /* Target frames per second (0 = unlimited) */
} endgine_config_t;
```

#### Callback Types

```c
typedef int (*endgine_init_func)(void);
typedef void (*endgine_update_func)(float delta_time);
typedef void (*endgine_render_func)(void);
typedef void (*endgine_cleanup_func)(void);
```

### Functions

#### `endgine_run`

Initialize and run the game loop.

```c
int endgine_run(const endgine_config_t *config);
```

**Parameters:**
- `config`: Pointer to configuration structure

**Returns:**
- 0 on success
- Non-zero on error

**Description:**
Initializes KallistiOS subsystems, calls the init callback, runs the main loop, and performs cleanup. The loop continues until `endgine_stop()` is called or the START button is pressed.

#### `endgine_stop`

Request the game loop to stop.

```c
void endgine_stop(void);
```

**Description:**
Can be called from within update or render callbacks to gracefully exit the game loop.

#### `endgine_get_fps`

Get the current frame rate.

```c
float endgine_get_fps(void);
```

**Returns:**
- Current FPS (updated every second)

#### `endgine_get_time`

Get total elapsed time since start.

```c
float endgine_get_time(void);
```

**Returns:**
- Time in seconds since `endgine_run()` was called

## Usage Example

```c
#include "endgine.h"
#include <kos.h>

static float player_x = 320.0f;
static float player_y = 240.0f;

static int game_init(void) {
    printf("Game initialized\n");
    return 0;
}

static void game_update(float delta_time) {
    /* Update game state */
    player_x += 100.0f * delta_time; /* Move 100 pixels per second */
}

static void game_render(void) {
    /* Render graphics using PowerVR */
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;
    
    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));
    
    /* Draw a quad at player position */
    /* ... vertex setup ... */
}

static void game_cleanup(void) {
    printf("Game cleanup\n");
}

int main(int argc, char **argv) {
    endgine_config_t config = {
        .init = game_init,
        .update = game_update,
        .render = game_render,
        .cleanup = game_cleanup,
        .target_fps = 60
    };
    
    return endgine_run(&config);
}
```

## Game Loop Flow

1. **Initialization Phase**
   - PowerVR subsystem initialized
   - User `init()` callback called
   - Timing system initialized

2. **Main Loop** (repeats until exit)
   - Calculate delta time
   - Update FPS counter
   - Check for START button press
   - Call user `update(delta_time)` callback
   - Wait for PowerVR ready
   - Begin PVR scene
   - Call user `render()` callback
   - Finish PVR scene
   - Limit frame rate if needed

3. **Cleanup Phase**
   - Call user `cleanup()` callback
   - Shutdown PowerVR subsystem

## Frame Timing

enDgine automatically calculates delta time between frames and passes it to the `update()` callback. This allows for frame-rate independent movement and animation:

```c
/* Move at 100 pixels per second regardless of frame rate */
position += velocity * delta_time;
```

## FPS Limiting

Set `target_fps` in the config to limit frame rate:

```c
config.target_fps = 60;  /* 60 FPS */
config.target_fps = 30;  /* 30 FPS */
config.target_fps = 0;   /* Unlimited */
```

## Input Handling

The START button automatically exits the game loop. For other input, use KallistiOS maple functions in your `update()` callback:

```c
static void game_update(float delta_time) {
    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    if (cont) {
        cont_state_t *state = (cont_state_t *)maple_dev_status(cont);
        if (state->buttons & CONT_A) {
            /* A button pressed */
        }
    }
}
```

## Best Practices

1. **Keep callbacks lightweight** - The main loop should run at 60 FPS
2. **Use delta time** - Always multiply velocities by delta_time for smooth movement
3. **Initialize in init()** - Don't do heavy initialization in update/render
4. **Clean up resources** - Release textures and memory in cleanup()
5. **Check return values** - Always check if `endgine_run()` returns an error

## Troubleshooting

### Black screen
- Ensure PVR lists are properly set up in render()
- Check that vertices are within screen bounds (0-640, 0-480)
- Verify texture formats are correct

### Slow frame rate
- Profile your update() and render() callbacks
- Reduce polygon count or texture sizes
- Check for unnecessary calculations in the loop

### Crashes
- Verify all pointers in config are valid
- Check for buffer overruns in render code
- Ensure texture memory is not exhausted

## License

See LICENSE file in the repository.
