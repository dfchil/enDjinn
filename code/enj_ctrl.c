#include <enDjinn/enj_ctrl.h>

// Note improved callback based state handling inspired by this code by
// darcagn/Eric Fradella
// https://gist.github.com/darcagn/eaf50e4b13ef9da7a8029dfaeafb75aa

alignas(32) static enj_ctrlr_state_t ctrlr_states_storage[MAPLE_PORT_COUNT] = {
    0};
alignas(32) static enj_ctrlr_state_t *ctrlr_states_refs[MAPLE_PORT_COUNT] = {0};
alignas(32) static maple_device_t *local_controllers[MAPLE_PORT_COUNT] = {0};

/* Called at init and then only as a callback when controller
   devices are connected or disconnected */
void scan_local_controllers(maple_device_t *__unused) {
  /* Clear existing controller status */
  for (int i = 0; i < 4; i++) {
    local_controllers[i] = NULL;
  }

  /* Loop through all available controllers
     and assign them to the proper ports */
  int i = 0;
  maple_device_t *cont;
  while ((cont = maple_enum_type(i, MAPLE_FUNC_CONTROLLER))) {
    local_controllers[cont->port] = cont;
    i++;
  }
}

void enj_ctrl_init_local_devices(void) {
  scan_local_controllers(NULL);
  maple_attach_callback(MAPLE_FUNC_CONTROLLER, scan_local_controllers);
  maple_detach_callback(MAPLE_FUNC_CONTROLLER, scan_local_controllers);
}

size_t enj_ctrl_states_length(void) {
  return sizeof(enj_abstract_ctrlr_t *) * MAPLE_PORT_COUNT;
}

size_t enj_ctrl_map_states(void) {
  size_t count = 0;
  for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
    maple_device_t *device = local_controllers[i];
    if (device && device->valid == true) {
      ctrlr_states_refs[i] = ctrlr_states_storage + i;
      cont_state_t *new_state = (cont_state_t *)maple_dev_status(device);
      ctrlr_states_refs[i]->state = new_state;
      if (new_state) {
        // Map the cont_state_t onto enj_ctrlr_state_t
        enj_ctrl_kos2enj_state(new_state, ctrlr_states_refs[i]);
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
  case ENJ_BUTTON_UP:
    return input ? ENJ_BUTTON_DOWN_THIS_FRAME : ENJ_BUTTON_UP;
    break;
  case ENJ_BUTTON_DOWN:
    return input ? ENJ_BUTTON_DOWN : ENJ_BUTTON_UP_THIS_FRAME;
    break;
  case ENJ_BUTTON_DOWN_THIS_FRAME:
    return input ? ENJ_BUTTON_DOWN : ENJ_BUTTON_UP_THIS_FRAME;
    break;
  case ENJ_BUTTON_UP_THIS_FRAME:
    return input ? ENJ_BUTTON_DOWN_THIS_FRAME : ENJ_BUTTON_UP;
    break;
  default:
    return ENJ_BUTTON_UP;
    break;
  }
}

void enj_ctrl_kos2enj_state(cont_state_t *c_state, enj_ctrlr_state_t *ctrlr) {
  if (c_state->a != ctrlr->button.A) {
    ctrlr->button.A = enj_update_button_state(ctrlr->button.A, c_state->a);
  }
  if (c_state->b != ctrlr->button.B) {
    ctrlr->button.B = enj_update_button_state(ctrlr->button.B, c_state->b);
  }
  if (c_state->x != ctrlr->button.X) {
    ctrlr->button.X = enj_update_button_state(ctrlr->button.X, c_state->x);
  }
  if (c_state->y != ctrlr->button.Y) {
    ctrlr->button.Y = enj_update_button_state(ctrlr->button.Y, c_state->y);
  }
  if (c_state->dpad_up != ctrlr->button.UP) {
    ctrlr->button.UP =
        enj_update_button_state(ctrlr->button.UP, c_state->dpad_up);
  }
  if (c_state->dpad_down != ctrlr->button.DOWN) {
    ctrlr->button.DOWN =
        enj_update_button_state(ctrlr->button.DOWN, c_state->dpad_down);
  }
  if (c_state->dpad_left != ctrlr->button.LEFT) {
    ctrlr->button.LEFT =
        enj_update_button_state(ctrlr->button.LEFT, c_state->dpad_left);
  }
  if (c_state->dpad_right != ctrlr->button.RIGHT) {
    ctrlr->button.RIGHT =
        enj_update_button_state(ctrlr->button.RIGHT, c_state->dpad_right);
  }
  if (c_state->start != ctrlr->button.START) {
    ctrlr->button.START =
        enj_update_button_state(ctrlr->button.START, c_state->start);
  }
  ctrlr->joyx = c_state->joyx;
  ctrlr->joyy = c_state->joyy;
  ctrlr->ltrigger = c_state->ltrig;
  ctrlr->rtrigger = c_state->rtrig;
}

enj_ctrlr_state_t **enj_ctrl_get_states(void);
void enj_read_controller(enj_abstract_ctrlr_t *ctrlref,
                         enj_ctrlr_state_t *cstate) {
  if (ctrlref != NULL && ctrlref->updatefun != NULL) {
    ctrlref->updatefun(ctrlref->state, cstate);
  }
}

int enj_ctrlr_button_combo_raw(uint32_t raw_buttons, uint32_t combo) {
  // all button in combo are pressed
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
  return enj_ctrlr_button_combo_raw(cstate->button.raw, combo->button.raw);
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