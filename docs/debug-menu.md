# Debug menu

[Documentation](README.md)

A menu that a button combination calls up over a running game. It freezes the
game by going on top of the mode stack, and it shows rows of settings with the
quick font and a help section at the bottom. Declarations are in
[`enj_debug_menu.h`](../include/enDjinn/enj_debug_menu.h); the
[debug menu example](../examples/enj_debug_menu/code/enj_debug_menu.c) uses
every kind of row.

## Building it in

Set `ENJ_DEBUG_MENU:=1` in `local.cfg.mk`. It turns on `ENJ_INJECT_QFONT` too,
which the menu draws with. Without the switch the header declares nothing, so
wrap calls in `#ifdef ENJ_DEBUG_MENU` in an application that builds both ways.
The Dreamcast binary then holds no menu code, and the frame loop has no check
for the combination. See [Configuration](configuration.md#feature-switches).

## Rows and the menu

A row is a name, a colon and its values. Rows point at the application's own
variables, and the menu changes them directly:

| Type | Field | Left and right |
| --- | --- | --- |
| `ENJ_MENU_CHOICE` | `.choice`: an index into `labels` | Step through the labels, wrapping around. |
| `ENJ_MENU_INT` | `.integer`: `min`, `max`, `step`, optional `format` | Add `step`, stopping at the limits. |
| `ENJ_MENU_FLOAT` | `.real`: `min`, `max`, `step`, optional `format` | Add `step`, stopping at the limits. |

A row with a NULL value pointer has no value and exists for `on_press` alone.
All labels of a choice are shown when they fit on the line, and the current one
is highlighted; otherwise only the current one is shown, as `< label >`.

```c
static int frame_mode;
static const char *const frame_modes[] = {"30fps", "60fps"};

static enj_menu_row_t rows[] = {
    {.name = "Framerate",
     .help = "60fps interpolates between two logic steps.",
     .type = ENJ_MENU_CHOICE,
     .choice = {.value = &frame_mode, .labels = frame_modes, .count = 2}},
};
static enj_debug_menu_t menu = {
    .title = "Debug", .rows = rows, .row_count = 1, .draw_behind = queue_game};

enj_debug_menu_set(&menu);
```

The help section shows the selected row's `help`, or the menu's own `help` for
a row without one. It wraps at spaces, breaks at `\n` and has room for three
lines. Rows that do not fit on the screen scroll with the selection.

Everything the menu points at is borrowed: the menu, rows and labels must stay
alive while it can open. `data` is passed to every callback.

## Opening and closing

Holding both triggers and pressing Up+X+Y opens the registered menu on any
controller. The triggers must be down before the last of the three buttons,
because `enj_ctrlr_button_combo_raw()` needs a button pressed this frame.
Nothing opens before `enj_debug_menu_set()` has been called, and
`enj_debug_menu_set(NULL)` turns the combination off. The combination is read
through enDjinn's controller layer: an application that sets `no_input_init`
must start it with `enj_ctrl_init_local_devices()`.

`enj_debug_menu_combo_set()` changes the buttons, in the form of
`enj_state_soft_reset_set()`. The trigger test belongs to the callback in
`enj_state_get()->debug_menu_open`, which the frame loop calls when the buttons
are pressed. Replace it after `enj_debug_menu_set()` to decide differently, and
call `enj_debug_menu_open()` from it, or from an application's own menu.

In the menu, Up and Down select a row, Left and Right change its value and keep
changing it while held, A presses it, and B or Start close the menu.

The menu closes once every button and trigger of that controller is released,
so that B does not reach the game as a new press. The game then runs in that
same frame. The soft reset combination is not the menu's: it closes the menu
at once, and the mode below sees it pressed in the next frame. An unplugged
controller also closes the menu at once.

## Freezing the game

The menu is a mode, so the game's updater is not called while it is open. What
keeps running is what does not depend on that updater: threads, interrupts and
the sound chip. A looping sound goes on playing unless `on_open` stops it, and
a game that measures real time should restart its clock in `on_close`.

`draw_behind` is called once per menu frame before the scene, where
`enj_render_list_add()` is allowed. Queue the frozen game there, and do
anything else that must go on while it is frozen. It must draw without
advancing the game. Without it the screen shows the background colour.

The menu goes into the translucent list after what `draw_behind` queued, and it
does not open if that list has no bins. It dims the screen and sets its own font
depth and scale, restoring them afterwards. It does not touch the quick font's
sprite header or colour.

The host backends clamp depth at z = 4, and an equal depth does not pass, so
game text at the font's default z of 10 is not dimmed there. The Dreamcast has
no such limit.

## Changes that take the frame away

`on_change` runs before the scene and is free to restart things, but the frozen
frame may depend on what it restarts. A runtime FSAA switch restarts the PVR and
frees every texture, and `draw_behind` would then draw a frame that no longer
exists. Set `apply_on_close` on such a row: its `on_change` waits until the menu
closes, and runs once however often the value changed. These run before
`on_close`, and the game then makes the next frame itself.

## Without the mode stack

`enj_debug_menu.c` needs neither `enj_state`, the mode stack nor enDjinn's
controller code. An application with its own frame loop links that file with
the quick font sources, keeps its own `enj_ctrlr_state_t`, and drives the menu
itself. The combination, the trigger test and the soft reset are then its own:

```c
if (combination_pressed) {
  enj_debug_menu_begin(&menu);
  menu_open = 1;
}
if (menu_open && enj_debug_menu_update(&menu, &controller)) {
  menu_open = 0;
}
// and in the translucent list, after everything else:
if (menu_open) {
  enj_debug_menu_draw(&menu);
}
```

`enj_debug_menu_set()` and `enj_debug_menu_open()` are in
`enj_debug_menu_mode.c`, and cannot be used this way.
