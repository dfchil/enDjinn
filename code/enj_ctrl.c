#include <enDjinn/enj_ctrl.h>

static cont_state_t state_backups[MAPLE_PORT_COUNT] = {0};

static enj_ctrlr_state_t ctrlr_states_storage[MAPLE_PORT_COUNT] = {0};
static enj_ctrlr_state_t *ctrlr_states_refs[MAPLE_PORT_COUNT] = {0};

int enj_ctrl_map_states(void) {
  int count = 0;
  for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
    maple_device_t *device = enj_maple_port_type(i, MAPLE_FUNC_CONTROLLER);

    if (device && device->valid == true) {
      ctrlr_states_refs[i] = ctrlr_states_storage + i;
      cont_state_t *state = (cont_state_t *)maple_dev_status(device);
      state_backups[i] = *state;
      ctrlr_states_refs[i]->state = state_backups + i;
      if (state) {
        // Map the cont_state_t onto enj_ctrlr_state_t
        enj_kos_state2ctrlrstate(state, ctrlr_states_refs[i]);
        ctrlr_states_refs[i]->portnum = i;
        count++;
      }
    } else {
      ctrlr_states_refs[i] = NULL;
    }
  }
  return count;
}

/* Return the first device of the requested type on port p */
maple_device_t *enj_maple_port_type(int p, uint32 func) {
  maple_device_t *dev;
  for (int u = 0; u < MAPLE_UNIT_COUNT; u++) {
    dev = maple_enum_dev(p, u);
    if (dev != NULL && (dev->info.functions & func)) {
      return dev;
    }
  }
  return NULL;
}

enj_ctrlr_state_t **enj_ctrl_get_states(void) { return ctrlr_states_refs; }

static inline uint8_t enj_update_button_state(uint8_t prev_btnstate,
                                              int input) {
  switch (prev_btnstate) {
  case BUTTON_UP:
    return input ? BUTTON_DOWN_THIS_FRAME : BUTTON_UP;
    break;
  case BUTTON_DOWN:
    return input ? BUTTON_DOWN : BUTTON_UP_THIS_FRAME;
    break;
  case BUTTON_DOWN_THIS_FRAME:
    return input ? BUTTON_DOWN : BUTTON_UP_THIS_FRAME;
    break;
  case BUTTON_UP_THIS_FRAME:
    return input ? BUTTON_DOWN_THIS_FRAME : BUTTON_UP;
    break;
  default:
    return BUTTON_UP;
    break;
  }
}

void enj_kos_state2ctrlrstate(cont_state_t *c_state, enj_ctrlr_state_t *ctrlr) {
  if (c_state->a != ctrlr->buttons.A) {
    ctrlr->buttons.A = enj_update_button_state(ctrlr->buttons.A, c_state->a);
  }
  if (c_state->b != ctrlr->buttons.B) {
    ctrlr->buttons.B = enj_update_button_state(ctrlr->buttons.B, c_state->b);
  }
  if (c_state->x != ctrlr->buttons.X) {
    ctrlr->buttons.X = enj_update_button_state(ctrlr->buttons.X, c_state->x);
  }
  if (c_state->y != ctrlr->buttons.Y) {
    ctrlr->buttons.Y = enj_update_button_state(ctrlr->buttons.Y, c_state->y);
  }
  if (c_state->dpad_up != ctrlr->buttons.UP) {
    ctrlr->buttons.UP =
        enj_update_button_state(ctrlr->buttons.UP, c_state->dpad_up);
  }
  if (c_state->dpad_down != ctrlr->buttons.DOWN) {
    ctrlr->buttons.DOWN =
        enj_update_button_state(ctrlr->buttons.DOWN, c_state->dpad_down);
  }
  if (c_state->dpad_left != ctrlr->buttons.LEFT) {
    ctrlr->buttons.LEFT =
        enj_update_button_state(ctrlr->buttons.LEFT, c_state->dpad_left);
  }
  if (c_state->dpad_right != ctrlr->buttons.RIGHT) {
    ctrlr->buttons.RIGHT =
        enj_update_button_state(ctrlr->buttons.RIGHT, c_state->dpad_right);
  }
  if (c_state->start != ctrlr->buttons.START) {
    ctrlr->buttons.START =
        enj_update_button_state(ctrlr->buttons.START, c_state->start);
  }
  ctrlr->joyx = c_state->joyx;
  ctrlr->joyy = c_state->joyy;
  ctrlr->ltrigger = c_state->ltrig;
  ctrlr->rtrigger = c_state->rtrig;
}

enj_ctrlr_state_t **enj_ctrl_get_states(void);
void enj_read_controller(enj_abstract_ctrlr_t *ctrlref,
                         enj_ctrlr_state_t *buttons) {
  if (ctrlref != NULL && ctrlref->updatefun != NULL) {
    ctrlref->updatefun(ctrlref->state, buttons);
  }
}

int enj_ctrlr_button_combo_raw(uint32_t raw_buttons, uint32_t combo) {
  // all buttons in combo are pressed
  if ((raw_buttons & combo) == combo) {
    // and at least one button from combo was pressed this frame
    if (raw_buttons & (combo << 1)) {
      return 1;
    }
  }
  return 0;
}

int enj_ctrlr_button_combo(enj_ctrlr_state_t *cstate,
                           enj_ctrlr_state_t *combo) {
  return enj_ctrlr_button_combo_raw(cstate->buttons.raw, combo->buttons.raw);
}

void enj_read_dreamcast_controller(void *ctrlr, enj_ctrlr_state_t *cstate) {
  if (ctrlr == NULL || cstate == NULL) {
    return;
  }
  enj_ctrlr_state_t *c_state =
      *(ctrlr_states_refs + ((enj_abstract_ctrlr_t *)ctrlr)->port);
  if (c_state == NULL) {
    return;
  }
  *cstate = *c_state;
}