#include <enDjinn/enj_ctrl.h>

#ifdef ENJ_TARGET_PC_ENDJINN
#include <SDL.h>
#include <stdio.h>
#endif

// Note improved callback based state handling inspired by this code by
// darcagn/Eric Fradella
// https://gist.github.com/darcagn/eaf50e4b13ef9da7a8029dfaeafb75aa

alignas(32) static enj_ctrlr_state_t ctrlr_states_storage[MAPLE_PORT_COUNT] = {
    0};
alignas(32) static enj_ctrlr_state_t *ctrlr_states_refs[MAPLE_PORT_COUNT] = {0};
alignas(32) static maple_device_t *local_controllers[MAPLE_PORT_COUNT] = {0};
#ifdef ENJ_TARGET_PC_ENDJINN
static enj_input_source_e input_action_sources[ENJ_INPUT_ACTION_CAPACITY] = {0};
static uint8_t input_action_states[ENJ_INPUT_ACTION_CAPACITY] = {0};
static SDL_GameController *host_game_controller = NULL;
#endif

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

#ifdef ENJ_TARGET_PC_ENDJINN

static void enj_host_refresh_game_controller(void) {
  if (host_game_controller != NULL &&
      !SDL_GameControllerGetAttached(host_game_controller)) {
    SDL_GameControllerClose(host_game_controller);
    host_game_controller = NULL;
  }
  if (host_game_controller != NULL) {
    return;
  }
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (!SDL_IsGameController(i)) {
      continue;
    }
    host_game_controller = SDL_GameControllerOpen(i);
    if (host_game_controller != NULL) {
      fprintf(stderr, "pc-enDjinn: controller connected: %s\n",
              SDL_GameControllerName(host_game_controller));
      return;
    }
  }
}

static int8_t enj_host_axis_to_i8(Sint16 axis) {
  const int deadzone = 6000;
  if (axis > -deadzone && axis < deadzone) {
    return 0;
  }
  int value = axis / 256;
  if (value < -127) {
    value = -127;
  } else if (value > 127) {
    value = 127;
  }
  return (int8_t)value;
}

static uint8_t enj_host_trigger_to_u8(Sint16 axis) {
  if (axis <= 0) {
    return 0u;
  }
  const int value = axis / 128;
  return (uint8_t)(value > 255 ? 255 : value);
}
static int enj_input_source_down(enj_input_source_e source) {
  const uint8_t *keys = SDL_GetKeyboardState(NULL);
  if (keys == NULL) {
    return 0;
  }
  switch (source) {
  case ENJ_INPUT_SOURCE_KEY_B: return keys[SDL_SCANCODE_B] != 0u;
  case ENJ_INPUT_SOURCE_KEY_E: return keys[SDL_SCANCODE_E] != 0u;
  case ENJ_INPUT_SOURCE_KEY_M: return keys[SDL_SCANCODE_M] != 0u;
  case ENJ_INPUT_SOURCE_KEY_R: return keys[SDL_SCANCODE_R] != 0u;
  case ENJ_INPUT_SOURCE_KEY_F1: return keys[SDL_SCANCODE_F1] != 0u;
  case ENJ_INPUT_SOURCE_KEY_F2: return keys[SDL_SCANCODE_F2] != 0u;
  case ENJ_INPUT_SOURCE_KEY_F3: return keys[SDL_SCANCODE_F3] != 0u;
  case ENJ_INPUT_SOURCE_KEY_F4: return keys[SDL_SCANCODE_F4] != 0u;
  default: return 0;
  }
}

static void enj_input_actions_update(void) {
  for (size_t i = 0u; i < ENJ_INPUT_ACTION_CAPACITY; i++) {
    input_action_states[i] = enj_update_button_state(
        input_action_states[i],
        enj_input_source_down(input_action_sources[i]));
  }
}

void enj_input_action_bind(enj_input_action_t action,
                           enj_input_source_e source) {
  if (action >= ENJ_INPUT_ACTION_CAPACITY) {
    return;
  }
  input_action_sources[action] = source;
  input_action_states[action] = ENJ_BUTTON_UP;
}

int enj_input_action_down(enj_input_action_t action) {
  if (action >= ENJ_INPUT_ACTION_CAPACITY) {
    return 0;
  }
  return input_action_states[action] == ENJ_BUTTON_DOWN ||
         input_action_states[action] == ENJ_BUTTON_DOWN_THIS_FRAME;
}

int enj_input_action_pressed(enj_input_action_t action) {
  return action < ENJ_INPUT_ACTION_CAPACITY &&
         input_action_states[action] == ENJ_BUTTON_DOWN_THIS_FRAME;
}
#endif

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

#ifdef ENJ_TARGET_PC_ENDJINN

void scan_local_controllers(maple_device_t *__unused) {}

void enj_ctrl_init_local_devices(void) {
  static maple_device_t keyboard_device = {
      .port = 0,
      .unit = 0,
      .valid = true,
      .info = {.functions = MAPLE_FUNC_CONTROLLER},
  };
  local_controllers[0] = &keyboard_device;
  ctrlr_states_refs[0] = ctrlr_states_storage;
  ctrlr_states_storage[0].portnum = 0;
  if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
    fprintf(stderr, "pc-enDjinn: SDL controller initialization failed: %s\n",
            SDL_GetError());
  } else {
    SDL_GameControllerEventState(SDL_ENABLE);
    enj_host_refresh_game_controller();
  }
}

size_t enj_ctrl_states_length(void) {
  return MAPLE_PORT_COUNT;
}

size_t enj_ctrl_map_states(void) {
  SDL_PumpEvents();
  enj_host_refresh_game_controller();
  const uint8_t *keys = SDL_GetKeyboardState(NULL);
  if (keys == NULL) {
    ctrlr_states_refs[0] = NULL;
    return 0u;
  }

  ctrlr_states_refs[0] = ctrlr_states_storage;
  cont_state_t state = {0};
  const bool has_pad = host_game_controller != NULL;
  const uint8_t pad_rtrigger = has_pad ? enj_host_trigger_to_u8(
      SDL_GameControllerGetAxis(host_game_controller,
                                SDL_CONTROLLER_AXIS_TRIGGERRIGHT)) : 0u;
  const uint8_t pad_ltrigger = has_pad ? enj_host_trigger_to_u8(
      SDL_GameControllerGetAxis(host_game_controller,
                                SDL_CONTROLLER_AXIS_TRIGGERLEFT)) : 0u;
  state.rtrig = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_W]
      ? 255u : pad_rtrigger;
  state.ltrig = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_S]
      ? 255u : pad_ltrigger;
  state.a = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_W] ||
      (has_pad && SDL_GameControllerGetButton(
          host_game_controller, SDL_CONTROLLER_BUTTON_A)) || state.rtrig > 16u;
  state.b = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_S] ||
      (has_pad && SDL_GameControllerGetButton(
          host_game_controller, SDL_CONTROLLER_BUTTON_B)) || state.ltrig > 16u;
  state.x = keys[SDL_SCANCODE_X] || (has_pad && SDL_GameControllerGetButton(
      host_game_controller, SDL_CONTROLLER_BUTTON_X));
  state.y = keys[SDL_SCANCODE_Y] || (has_pad && SDL_GameControllerGetButton(
      host_game_controller, SDL_CONTROLLER_BUTTON_Y));
  state.start = keys[SDL_SCANCODE_RETURN] ||
      (has_pad && SDL_GameControllerGetButton(
          host_game_controller, SDL_CONTROLLER_BUTTON_START));
  state.dpad_up = keys[SDL_SCANCODE_UP] ||
      (has_pad && SDL_GameControllerGetButton(
          host_game_controller, SDL_CONTROLLER_BUTTON_DPAD_UP));
  state.dpad_down = keys[SDL_SCANCODE_DOWN] ||
      (has_pad && SDL_GameControllerGetButton(
          host_game_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN));
  state.dpad_left = keys[SDL_SCANCODE_LEFT] ||
      (has_pad && SDL_GameControllerGetButton(
          host_game_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT));
  state.dpad_right = keys[SDL_SCANCODE_RIGHT] ||
      (has_pad && SDL_GameControllerGetButton(
          host_game_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
  state.joyx = keys[SDL_SCANCODE_A] ? -127 :
      (keys[SDL_SCANCODE_D] ? 127 :
       (has_pad ? enj_host_axis_to_i8(SDL_GameControllerGetAxis(
           host_game_controller, SDL_CONTROLLER_AXIS_LEFTX)) : 0));
  state.joyy = keys[SDL_SCANCODE_W] ? -127 :
      (keys[SDL_SCANCODE_S] ? 127 :
       (has_pad ? enj_host_axis_to_i8(SDL_GameControllerGetAxis(
           host_game_controller, SDL_CONTROLLER_AXIS_LEFTY)) : 0));
  enj_ctrl_kos2enj_state(&state, ctrlr_states_refs[0]);
  ctrlr_states_refs[0]->portnum = 0;
  enj_input_actions_update();

  for (int i = 1; i < MAPLE_PORT_COUNT; i++) {
    ctrlr_states_refs[i] = NULL;
  }
  return 1u;
}

maple_device_t *enj_maple_port_type(int p, uint32 func) {
  if (p == 0 && local_controllers[0] != NULL &&
      (local_controllers[0]->info.functions & func)) {
    return local_controllers[0];
  }
  return NULL;
}

#else

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
  return MAPLE_PORT_COUNT;
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

#endif

enj_ctrlr_state_t **enj_ctrl_get_states(void) { return ctrlr_states_refs; }

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
