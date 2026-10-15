# Rumble and VMU

[Documentation](README.md)

Rumble has a small API with per-port rate limiting. VMUs have no enDjinn API;
this page says what enDjinn does with them anyway. Rumble declarations are in
[`enj_rumble.h`](../include/enDjinn/enj_rumble.h), and the
[rumbles example](../examples/enj_rumbles/code/enj_rumbles.c) is an
interactive effect editor.

## Rumble

`enj_rumble_effect_set(port, effect)` queues a KOS `purupuru_effect_t` for a
port; `enj_rumble_effect_set_raw()` takes the effect's raw 32-bit value. The
queued effect is sent to the rumble pack at the end of the frame. At most one
effect is sent per port every `n` frames, where `n` is set with
`enj_rumble_rate_limit_set()` (default 1). A newer effect replaces one that is
still waiting.

The return value is a set of flags:

| Flag | Meaning |
| --- | --- |
| `ENJ_RUMBLE_SET` | The effect was queued. |
| `ENJ_RUMBLE_NO_DEVICE` | No rumble pack on that port; nothing was queued. |
| `ENJ_RUMBLE_RATE_LIMITED` | Queued, but the port is still inside its rate limit. |
| `ENJ_RUMBLE_OVERWROTE_PREVIOUS` | Queued, replacing an effect not yet sent. |

To stop the motor, send an effect with only `motor = 1` set, as the example
does. `enj_rumble_states_get()` returns the rumble device per port (null where
there is none). enDjinn stops all rumble packs on shutdown.

On pc-enDjinn and web-enDjinn, rumble-capable game controllers report a rumble
pack. The effect's power and frequency drive the controller's low- and
high-frequency motors through SDL or the browser Gamepad vibration actuator.

## VMU

enDjinn has no VMU screen or save API. It touches VMUs in two places:

- on shutdown it clears the LCD of every attached VMU;
- `enj_ctrl.h`'s `enj_maple_port_type(port, MAPLE_FUNC_LCD)` finds a VMU
  screen on a port, for use with the KOS `vmufb_*` functions.

Applications use KOS directly for VMU screens and save files. On the host
backends, KOS file calls on paths under `/vmu/` are redirected: pc-enDjinn
writes them below `vmu/` in the working directory, and web-enDjinn keeps them
in an IndexedDB-backed `/vmu` directory that survives page reloads. VMU
screens are not shown on either host backend.
