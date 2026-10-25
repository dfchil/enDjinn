# Rumble and VMU

[Documentation](README.md)

Rumble has a small API with per-port rate limiting, and VMUs have a save-file
API. Rumble declarations are in
[`enj_rumble.h`](../include/enDjinn/enj_rumble.h), and the
[rumbles example](../examples/enj_rumbles/code/enj_rumbles.c) is an
interactive effect editor. Save declarations are in
[`enj_vmu.h`](../include/enDjinn/enj_vmu.h).

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

## VMU saves

An application has one save: a block of its own memory, described by an
`enj_vmu_save_t` with the file name, the two descriptions the file menus
show, an application id and an optional 16-colour icon (1 to 3 frames of
32x32 pixels, 4 bits each). On the Dreamcast it is stored as a VMS file whose
CRC is checked on load.

```c
static enj_vmu_save_t save = {
    .name = "MYGAME", .desc_short = "My Game", .desc_long = "My Game save",
    .app_id = "MYGAME", .icon_palette = icon_pal, .icon_frames = icon,
    .icon_frame_count = 1, .data = &game_save, .size = sizeof(game_save),
};

if (enj_vmu_save_load(&save) != ENJ_VMU_LOADED) {
  /* start from defaults */
}
enj_vmu_save_start(&save, 500);

/* whenever the game changes its save */
enj_vmu_save_lock();
game_save.best_lap = lap;
enj_vmu_save_unlock();
enj_vmu_save_changed();
```

`enj_vmu_save_load()` reads the save from the first VMU that holds it and
says what it found. `enj_vmu_save_write()` writes it at once and blocks for the
maple transfers. `enj_vmu_save_start()` starts a background saver instead: a
low-priority thread that writes a copy once the save has not changed for the
given number of milliseconds, so the game thread never waits for the VMU. A
failed write is tried again, and a VMU plugged in after the load that already
holds a save is left alone. For a reset or quit, `enj_vmu_save_exit_after_flush()`
has the saver write waiting changes and then exit; it returns 0 when there is
nothing to save.

On the host backends the save is a plain file of the data alone, written at
once on every `enj_vmu_save_changed()`.

## VMU screens

enDjinn has no VMU screen API. It touches VMU screens in two places:

- on shutdown it clears the LCD of every attached VMU;
- `enj_ctrl.h`'s `enj_maple_port_type(port, MAPLE_FUNC_LCD)` finds a VMU
  screen on a port, for use with the KOS `vmufb_*` functions.

Applications use KOS directly for VMU screens. On the host
backends, KOS file calls on paths under `/vmu/` are redirected: pc-enDjinn
writes them below `vmu/` in the working directory, and web-enDjinn keeps them
in an IndexedDB-backed `/vmu` directory that survives page reloads. VMU
screens are not shown on either host backend.
