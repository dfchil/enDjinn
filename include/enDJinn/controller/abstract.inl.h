#include <enDJinn/controller/abstract.h>

static ctrl_box_t ctrl_box = {
    .current_ctrl_frame = 0,
    .current_render_frame = 0,
    .num_players = 8,
    .replay = NULL,
    .current_replay = 0,
    .serialfactory = 32,
};

ctrl_box_t *get_ctrl_box(void) { return &ctrl_box; }

static inline uint8_t update_button_state(uint8_t prev_btnstate, int input) {
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

void cont_state_onto_ctrlstate(cont_state_t *c_state,
                               controller_state_t *ctrlr) {
  if (c_state->a != ctrlr->BTN_A) {
    ctrlr->BTN_A = update_button_state(ctrlr->BTN_A, c_state->a);
  }
  if (c_state->b != ctrlr->BTN_B) {
    ctrlr->BTN_B = update_button_state(ctrlr->BTN_B, c_state->b);
  }
  if (c_state->x != ctrlr->BTN_X) {
    ctrlr->BTN_X = update_button_state(ctrlr->BTN_X, c_state->x);
  }
  if (c_state->y != ctrlr->BTN_Y) {
    ctrlr->BTN_Y = update_button_state(ctrlr->BTN_Y, c_state->y);
  }
  if (c_state->dpad_up != ctrlr->UP) {
    ctrlr->UP = update_button_state(ctrlr->UP, c_state->dpad_up);
  }
  if (c_state->dpad_down != ctrlr->DOWN) {
    ctrlr->DOWN = update_button_state(ctrlr->DOWN, c_state->dpad_down);
  }
  if (c_state->dpad_left != ctrlr->LEFT) {
    ctrlr->LEFT = update_button_state(ctrlr->LEFT, c_state->dpad_left);
  }
  if (c_state->dpad_right != ctrlr->RIGHT) {
    ctrlr->RIGHT = update_button_state(ctrlr->RIGHT, c_state->dpad_right);
  }
  if (c_state->start != ctrlr->START) {
    ctrlr->START = update_button_state(ctrlr->START, c_state->start);
  }
  ctrlr->joyx = c_state->joyx;
  ctrlr->joyy = c_state->joyy;
  ctrlr->ltrigger = c_state->ltrig;
  ctrlr->rtrigger = c_state->rtrig;
}

void control_next_frame(ctrl_box_t *_ctrl_box) {
  _ctrl_box->current_render_frame++;
}
controller_state_t **get_ctrlr_states(void);
void read_controller(abstract_controller_t *ctrlref,
                     controller_state_t *buttons) {
  switch (ctrlref->type) {
  case NO_CONTROLLER_E:
  case SERIAL_CABLE_E:
  case NETWORK_E:
    break;
  case DREAMCAST_STD_E:
    controller_state_t* t_state = get_ctrlr_states()[ctrlref->dc_ctrlr.offset];
    if (t_state) {
      *buttons = *t_state;
    } else {
      DEBUG_PRINT("Controller unavailable on port %d\n", ctrlref->dc_ctrlr.offset);
    }

    break;
  default:
    DEBUG_PRINT("Unknown controller type\n");
    break;
  }
}
