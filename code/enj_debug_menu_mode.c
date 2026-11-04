#ifdef ENJ_DEBUG_MENU

#include <enDjinn/enj_debug_menu.h>
#include <enDjinn/enj_defs.h>
#include <enDjinn/enj_mode.h>
#include <enDjinn/enj_render.h>
#include <enDjinn/enj_state.h>

#define ENJ_MENU_TRIGGER_DOWN 128

static enj_debug_menu_t *registered_menu;

static void enj_debug_menu_draw_item(void *data) {
  enj_debug_menu_draw(data);
}

/** The soft reset is no business of the menu, which has no_soft_reset for that
 * reason: a soft reset skips its closing. It is the mode below's, and this
 * is also how the host backends quit. Forgetting the buttons makes them a new
 * press next frame, when that mode is back on top.
 * @return 1 when the combination was pressed */
static int enj_debug_menu_pass_soft_reset(void) {
  enj_state_t *state = enj_state_get();
  enj_ctrlr_state_t **cstates = enj_ctrl_get_states();
  int pressed = 0;

  if (!state->flags.soft_reset_enabled) {
    return 0;
  }
  for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
    if (cstates[i] && enj_ctrlr_button_combo_raw(cstates[i]->button.raw,
                                                 state->exit_pattern)) {
      cstates[i]->button.raw = 0;
      pressed = 1;
    }
  }
  return pressed;
}

static void enj_debug_menu_updater(void *data) {
  enj_debug_menu_t *menu = data;
  enj_ctrlr_state_t *cstate = enj_debug_menu_pass_soft_reset()
                                  ? NULL
                                  : enj_ctrl_get_states()[menu->port];

  if (enj_debug_menu_update(menu, cstate)) {
    enj_mode_pop();
    // The game makes this frame itself: a held back on_change may just have
    // taken away what draw_behind would have needed.
    enj_mode_t *game = enj_mode_get();
    game->mode_updater(game->data);
    return;
  }
  if (menu->draw_behind != NULL) {
    menu->draw_behind(menu->data);
  }
  enj_render_list_add(PVR_LIST_TR_POLY, enj_debug_menu_draw_item, menu);
}

static enj_mode_t menu_mode = {
    .name = "debug menu",
    .mode_updater = enj_debug_menu_updater,
    .no_soft_reset = 1,
};

int enj_debug_menu_open(enj_ctrlr_state_t *cstate) {
  if (registered_menu == NULL || cstate == NULL ||
      enj_mode_get() == &menu_mode) {
    return 0;
  }
  // A list without bins is never opened: the game would freeze behind nothing.
  if (enj_state_get()->video.pvr_params.opb_sizes[PVR_LIST_TR_POLY] ==
      PVR_BINSIZE_0) {
    ENJ_DEBUG_PRINT("The debug menu needs bins in the translucent list\n");
    return 0;
  }
  menu_mode.data = registered_menu;
  if (!enj_mode_push(&menu_mode)) {
    return 0;
  }
  // The menu updates in this same frame and must not see the combination.
  cstate->button.raw &= ~(enj_state_get()->debug_menu_pattern << 1);
  registered_menu->port = cstate->portnum;
  enj_debug_menu_begin(registered_menu);
  return 1;
}

/** The combination's buttons were pressed: the triggers make it complete. */
static void enj_debug_menu_open_on_triggers(enj_ctrlr_state_t *cstate) {
  if (cstate->ltrigger >= ENJ_MENU_TRIGGER_DOWN &&
      cstate->rtrigger >= ENJ_MENU_TRIGGER_DOWN) {
    enj_debug_menu_open(cstate);
  }
}

void enj_debug_menu_set(enj_debug_menu_t *menu) {
  registered_menu = menu;
  enj_state_get()->debug_menu_open =
      menu != NULL ? enj_debug_menu_open_on_triggers : NULL;
}

void enj_debug_menu_combo_set(uint32_t pattern) {
  enj_state_get()->debug_menu_pattern = pattern;
}

#endif // ENJ_DEBUG_MENU
