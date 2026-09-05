# Target support

Dreamcast/KallistiOS is enDjinn's reference target. `pc-endjinn` and
`web-endjinn` are source-compatible development backends, not Dreamcast
emulators. They implement the engine-facing subset needed by the examples and
known applications.

| Capability | Dreamcast | PC | Web |
| --- | --- | --- | --- |
| Public C runtime and mode loop | Reference | Supported | Supported |
| Opaque, punch-through, translucent render lists | Native PVR | Supported subset | Supported subset |
| Colored and textured projected geometry | Native PVR | Supported subset | Supported subset |
| Opaque and translucent modifier volumes | Native PVR | Depth-aware emulation | Depth-aware emulation |
| Generated textures, fonts, and sounds | Supported | Same generated assets | Same generated assets |
| Controllers | Maple | SDL keyboard/gamepads | SDL/browser gamepads |
| Rumble | Purupuru | SDL gamepad rumble | Gamepad actuator when available |
| VMU save paths | Native | Host-file redirection | Virtual filesystem |
| VMU LCD | Native | Placeholder | Placeholder |
| Audio | KOS sound | PCM through SDL | PCM through SDL |
| Function/sampling profiling | Supported | — | — |
| Automated framebuffer capture | — | Backend diagnostic | Browser test harness |

The host rows describe compatibility, not cycle accuracy. Exact packet,
texture, modifier, input, and placeholder behavior is listed in the
[PC backend support document](../backends/pc-endjinn/SUPPORTED.md). Browser
setup and WebGL-specific behavior are documented in the
[Web backend README](../backends/web-endjinn/README.md).
