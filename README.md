# enDgine
A gameplay loop driver for the Dreamcast using KallistiOS

## Overview
enDgine provides a simple main function that initializes the Dreamcast with the KallistiOS system, sets up the graphics (PowerVR) and controller subsystems, and provides a basic game loop structure.

## Features
- KallistiOS initialization
- PowerVR graphics system setup
- Controller input handling
- Basic game loop with rendering pipeline
- Clean shutdown and cleanup

## Prerequisites
- KallistiOS toolchain installed and configured
- Set `KOS_BASE` environment variable to your KallistiOS installation directory
- `kos-cc` compiler in your PATH

## Building
```bash
make
```

This will produce `enDgine.elf` executable.

## Running
To run in an emulator (requires lxdream):
```bash
make run
```

Or use any Dreamcast emulator that supports ELF files.

## Usage
The main game loop is controlled via the controller:
- **START button**: Exit the application

## Code Structure
- `main.c`: Main entry point with KallistiOS initialization and game loop
- `Makefile`: Build configuration for KallistiOS toolchain

## License
See LICENSE file for details.
