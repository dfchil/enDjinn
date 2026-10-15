# Input

[Documentation](README.md)

enDjinn reads the local Maple controllers once per frame and keeps a small
per-port state with press and release transitions. Declarations are in
[`enj_ctrl.h`](../include/enDjinn/enj_ctrl.h); the
[controls example](../examples/enj_controls/code/enj_controls.c) shows them in
use.

## Controller states

`enj_ctrl_get_states()` returns an enDjinn-owned array with
`MAPLE_PORT_COUNT` entries. A null entry means that no controller is connected
at that port. Non-null entries are refreshed once per frame before the mode
updater runs and remain owned by enDjinn. Connection changes are picked up
through Maple attach and detach callbacks.

Each `enj_ctrlr_state_t` holds:

- `button`: A, B, X, Y, the D-pad and Start, two bits each, also readable
  together as `button.raw`;
- `joyx`, `joyy`: the analogue stick as signed bytes;
- `ltrigger`, `rtrigger`: the triggers as unsigned bytes;
- `portnum`: the Maple port;
- `state`: the KOS `cont_state_t` it was read from.

The `rumble` and `vmu` bit-fields are not filled in by enDjinn.

## Button states

Each button is one of `ENJ_BUTTON_UP`, `ENJ_BUTTON_DOWN`,
`ENJ_BUTTON_DOWN_THIS_FRAME` or `ENJ_BUTTON_UP_THIS_FRAME`. The low bit is
"held now" and the high bit "changed this frame", so a press is visible for
exactly one frame without callbacks:

```c
enj_ctrlr_state_t **ports = enj_ctrl_get_states();
if (ports[0] && ports[0]->button.A == ENJ_BUTTON_DOWN_THIS_FRAME) {
  /* A was pressed this frame */
}
```

`enj_ctrlr_button_combo(state, combo)` is true when every button set to
`ENJ_BUTTON_DOWN` in `combo` is held and at least one of them was pressed this
frame, which suits chords. `enj_ctrlr_button_combo_raw()` takes the same test
on raw bit patterns; the bit layout is listed in the header. The soft-reset
combination described in [Modes and the frame loop](modes.md#soft-reset) uses
this test.

## Abstract controllers

`enj_abstract_ctrlr_t` pairs a port name with an `updatefun(state, dest)` that
fills an `enj_ctrlr_state_t`. It lets a bot, a replay stream or a remote
console feed code that already consumes controller states.
`enj_read_controller()` calls the function. The frame loop does not: it reads
only local Maple controllers, so the application calls
`enj_read_controller()` itself. Port names beyond D (`ENJ_PORT_E` and up) are
reserved for such sources.

## Host backends

pc-enDjinn and web-enDjinn implement the same Maple controller interface on
SDL. The keyboard drives port A, so port A always reports a controller, and
up to four game controllers are mapped to ports A–D in connection order. The
key bindings are listed in the
[pc-enDjinn README](../backends/pc-endjinn/README.md#runtime-controls).
