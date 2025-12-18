#include <enDjinn/enj_enDjinn.h>
#include <dc/maple/purupuru.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  purupuru_effect_t effect;
  const char* description;
} baked_pattern_t;

typedef struct {
  struct {
    uint32_t no_controller : 1;
    uint32_t no_rumbles : 1;
    uint32_t active_controller : 2;
    uint32_t catalog_index: 4;
    int32_t cursor_pos : 5;
    int32_t loaded_pattern : 5;
    uint32_t reserved : 14;
  };
  purupuru_effect_t effect;
  maple_device_t* purudev;

} RAT_state_t;

static const baked_pattern_t catalog[] = {
    {.effect = {.cont = false, .motor = 1, .fpow = 7, .freq = 26, .inc = 1},
     .description = "Basic Thud (simple .5s jolt)"},
    {.effect = {.cont = true, .motor = 1, .fpow = 1, .freq = 7, .inc = 49},
     .description = "Car Idle (69 Mustang)"},
    {.effect = {.cont = false,
                .motor = 1,
                .fpow = 7,
                .conv = true,
                .freq = 21,
                .inc = 38},
     .description = "Car Idle (VW beetle)"},
    {.effect = {.cont = false,
                .motor = 1,
                .fpow = 7,
                .conv = true,
                .freq = 57,
                .inc = 51},
     .description = "Eathquake (Vibrate, and fade out)"},
    {.effect = {.cont = true, .motor = 1, .fpow = 1, .freq = 40, .inc = 5},
     .description = "Helicopter"},
    {.effect = {.cont = false, .motor = 1, .fpow = 2, .freq = 7, .inc = 0},
     .description = "Ship's Thrust (as in AAC)"},
};

static const char* fieldnames[] = {"cont", "res",  "motor", "bpow", "div",
                                   "fpow", "conv", "freq",  "inc"};
static const int num_fields = sizeof(fieldnames) / sizeof(fieldnames[0]);

/* motor cannot be 0 (will generate error on official hardware), but we can set
 * everything else to 0 for stopping */
static const purupuru_effect_t rumble_stop = {.motor = 1};


static inline uint8_t offset2field(int offset, RAT_state_t *state) {
  switch (offset) {
    case 0:
      return state->effect.cont;  // cont
    case 1:
      return state->effect.res;  // res
    case 2:
      return state->effect.motor;  // motor
    case 3:
      return state->effect.bpow;  // bpow
    case 4:
      return state->effect.div;  // div
    case 5:
      return state->effect.fpow;  // fpow
    case 6:
      return state->effect.conv;  // conv
    case 7:
      return state->effect.freq;  // freq
    case 8:
      return state->effect.inc;  // inc
    default:
      return -1;
  }
}

static inline void alter_field_at_offset(int offset, int delta, RAT_state_t *state) {
  switch (offset) {
    case 0:
      state->effect.cont = !state->effect.cont;  // cont
      break;
    case 1:
      break;  // res (reserved, cannot be changed)
    case 2:
      state->effect.motor = (state->effect.motor + delta) & 0xf;  // motor
      if (state->effect.motor == 0) state->effect.motor = 1;      // motor cannot be zero
      break;
    case 3:
      state->effect.bpow = (state->effect.bpow + delta) & 0x7;  // bpow
      if (state->effect.bpow)
        state->effect.fpow = 0;  // cannot have both forward and backward power
      break;
    case 4:
      state->effect.div = !state->effect.div;  // div
      if (state->effect.conv && state->effect.div)
        state->effect.conv = false;  // cannot have both convergent and divergent
      break;
    case 5:
      state->effect.fpow = (state->effect.fpow + delta) & 0x7;  // fpow
      if (state->effect.fpow)
        state->effect.bpow = 0;  // cannot have both forward and backward power
      break;
    case 6:
      state->effect.conv = !state->effect.conv;  // conv
      if (state->effect.conv && state->effect.div)
        state->effect.div = false;  // cannot have both convergent and divergent
      break;
    case 7:
      state->effect.freq = (state->effect.freq + delta) & 0xff;  // freq
      break;
    case 8:
      state->effect.inc = (state->effect.inc + delta) & 0xff;  // inc
      break;
    default:
      break;
  }
}

void render(void* data) {
  RAT_state_t* state = (RAT_state_t*)data;

  if (state->no_controller) {
    enj_qfont_write("Please attach a controller to port A!", 20,
                    20, PVR_LIST_PT_POLY);
    return;
  }
  if (state->no_rumbles) {
    enj_qfont_write("Please attach a rumbler to controller in port A!",
                    20, 20, PVR_LIST_PT_POLY);
    return;
  }

#define STRBUFSIZE 64
#define MARGIN_LEFT 10
  char str_buffer[STRBUFSIZE];

  enj_qfont_color_set(0xff, 0xc0, 0x10); /* gold */
  enj_font_scale_set(3);
  const char* title = "Rumble Accessory Tester";
  int twidth =
      enj_font_string_width(title, enj_qfont_get_header());
  int textpos_x = (vid_mode->width - twidth) >> 1;
  int textpos_y = 4;
  enj_qfont_write(title, textpos_x, textpos_y,
                  PVR_LIST_PT_POLY);
  enj_font_scale_set(1);

  /* Start drawing the changeable section of the screen */
  textpos_y += 4 * enj_qfont_get_header()->line_height;

  textpos_x = MARGIN_LEFT;
  enj_qfont_color_set(255, 255, 255); /* White */
  enj_qfont_write("Effect as hex value:", textpos_x, textpos_y, PVR_LIST_PT_POLY);
  enj_qfont_color_set(255, 0, 255); /* Magenta */
  snprintf(str_buffer, STRBUFSIZE, "0x%08lx", state->effect.raw);
  textpos_x = 170;
  enj_qfont_write(str_buffer, textpos_x, textpos_y, PVR_LIST_PT_POLY);
  textpos_y += enj_qfont_get_header()->line_height *2;
  textpos_x = MARGIN_LEFT;

  enj_qfont_color_set(255, 255, 255); /* White */
  enj_qfont_write("Effect as fields:", textpos_x, textpos_y, PVR_LIST_PT_POLY);
  textpos_y += enj_qfont_get_header()->line_height;
  
  enj_qfont_color_set(0x14, 0xaf, 255); /* Light Blue */
  for (int i = 0; i < num_fields; i++) {
    enj_qfont_write(fieldnames[i], textpos_x + 60 * i, textpos_y,
                    PVR_LIST_PT_POLY);
  }
  textpos_y += enj_qfont_get_header()->line_height;

  for (int i = 0; i < num_fields; i++) {
    if (state->cursor_pos == i)
      enj_qfont_color_set(255, 0, 0); /* Red */
    else
      enj_qfont_color_set(255, 255, 255); /* White */

    snprintf(str_buffer, STRBUFSIZE, " %u ", offset2field(i, &*state));
    enj_qfont_write(str_buffer, textpos_x + 60 * i, textpos_y,
                    PVR_LIST_PT_POLY);
  }

  textpos_y += 20;

  textpos_y += enj_qfont_get_header()->line_height;
  textpos_x = MARGIN_LEFT;
  enj_qfont_color_set(255, 255, 255); /* White */
  enj_qfont_write("Field description:", textpos_x, textpos_y, PVR_LIST_PT_POLY);
  enj_qfont_color_set(255, 0, 0); /* RED */
  textpos_x = 170;
  textpos_x += enj_qfont_write(" [", textpos_x, textpos_y, PVR_LIST_PT_POLY);
  textpos_x += enj_qfont_write(fieldnames[state->cursor_pos], textpos_x, textpos_y,
                               PVR_LIST_PT_POLY);
  enj_qfont_write("]", textpos_x, textpos_y, PVR_LIST_PT_POLY);
  textpos_y += enj_qfont_get_header()->line_height;
  textpos_x = MARGIN_LEFT * 3;

  enj_qfont_color_set(255, 255, 255); /* White */
  const char* field_descriptions[] = {
      // note that each description is 2 lines, some empty
      "Continuous Vibration. When set vibration will "
      "continue until stopped",
      "",

      "Reserved. Always 0s",
      "also will not be shown.",

      "Motor number. 0 will cause an error. 1 is the "
      "typical setting. 4-bits.",
      "",

      "Backward direction (- direction) intensity setting "
      "bits.",
      "0 stops vibration. Exclusive with .fpow. Field is "
      "3-bits.",

      "Divergent vibration. Make the rumble stronger until "
      "it stops.",
      "Exclusive with .conv.",

      "Forward direction (+ direction) intensity setting "
      "bits.",
      "0 stops vibration. Exclusive with .bpow. Field is "
      "3-bits.",

      "Convergent vibration. Make the rumble weaker until "
      "it stops.",
      "Exclusive with .div.",

      "Vibration frequency. For most purupuru the range is 4-59.",
      "Field is 8-bits.",

      "Vibration inclination period setting bits. Field is "
      "8-bits.",
      "",

      "Setting .inc == 0 when .conv or .div are set "
      "results in error.",
      ""};
  enj_qfont_write(field_descriptions[state->cursor_pos * 2], textpos_x, textpos_y,
                  PVR_LIST_PT_POLY);
  textpos_y += enj_qfont_get_header()->line_height;
  enj_qfont_write(field_descriptions[state->cursor_pos * 2 + 1], textpos_x, textpos_y,
                  PVR_LIST_PT_POLY);
  if (state->loaded_pattern >= 0) {
    textpos_y = 240;
    textpos_x = MARGIN_LEFT;
    enj_qfont_write("Loaded baked pattern:", textpos_x, textpos_y,
                    PVR_LIST_PT_POLY);
    enj_qfont_color_set(0, 255, 0); /* Green */
    textpos_y += enj_qfont_get_header()->line_height;
    enj_qfont_write(catalog[state->loaded_pattern].description, textpos_x, textpos_y,
                    PVR_LIST_PT_POLY);
    textpos_y += enj_qfont_get_header()->line_height;
  }

  /* Draw the bottom half of the screen and finish it up. */
  textpos_y = 344;
  textpos_x = MARGIN_LEFT;

  enj_qfont_color_set(0xff, 0xc0, 0x10); /* gold */
  enj_qfont_write("Instructions:", textpos_x, textpos_y, PVR_LIST_PT_POLY);
  textpos_y += enj_qfont_get_header()->line_height;
  enj_qfont_color_set(255, 255, 255); /* White */
  const char* instructions[] = {"Press left/right to switch field.",
                                "Press up/down to change values.",
                                "Press A to send effect to rumblepack.",
                                "Press B to stop rumble.",
                                "Press X for next baked pattern.",
                                "Press START to end program."};

  for (size_t i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++) {
    enj_qfont_write(instructions[i], textpos_x, textpos_y, PVR_LIST_PT_POLY);
    textpos_y += enj_qfont_get_header()->line_height;
  }
}

void main_mode_updater(void* data) {
  do {
    RAT_state_t* state = (RAT_state_t*)data;

    // neeeds to be at least one controller with a rumble pack
    enj_ctrlr_state_t** ctrl_states = enj_ctrl_get_states();
    maple_device_t** rumble_states = enj_rumble_states_get();

    state->no_controller = 1;
    state->no_rumbles = 1;
    state->purudev = NULL;


    for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
      if (ctrl_states[i] != NULL) {
        state->no_controller = 0;
      }
      if (rumble_states[i] != NULL) {
        state->no_rumbles = 0;
        state->purudev = rumble_states[i];
        state->active_controller = i;
      }
    }
    if (state->no_controller || state->no_rumbles) {
      break;
    }

    if (ctrl_states[state->active_controller]->button.LEFT ==
        ENJ_BUTTON_DOWN_THIS_FRAME) {
      state->cursor_pos = state->cursor_pos - 1;
      if (state->cursor_pos < 0)
        state->cursor_pos = num_fields - 1;
      if (state->cursor_pos == 1) state->cursor_pos = 0;
    }

    if (ctrl_states[state->active_controller]->button.RIGHT ==
        ENJ_BUTTON_DOWN_THIS_FRAME) {
      state->cursor_pos = (state->cursor_pos + 1) % num_fields;
      if (state->cursor_pos == 1) state->cursor_pos = 2;
    }

    int delta = ctrl_states[state->active_controller]->button.UP ==
                        ENJ_BUTTON_DOWN_THIS_FRAME
                    ? 1
                : ctrl_states[state->active_controller]->button.DOWN ==
                        ENJ_BUTTON_DOWN_THIS_FRAME
                    ? -1
                    : 0;

    if (delta) {
      alter_field_at_offset(state->cursor_pos, delta, state);
      state->loaded_pattern = -1;  // custom pattern, not from catalog
    }

    if (ctrl_states[state->active_controller]->button.X ==
        ENJ_BUTTON_DOWN_THIS_FRAME) {
      state->effect = catalog[state->catalog_index].effect;
      state->loaded_pattern = state->catalog_index;
      state->catalog_index++;

      if (state->catalog_index >= sizeof(catalog) / sizeof(baked_pattern_t))
        state->catalog_index = 0;
    }

    if (ctrl_states[state->active_controller]->button.A ==
        ENJ_BUTTON_DOWN_THIS_FRAME) {
      /* We print these out to make it easier to track the options chosen
       */
      ENJ_DEBUG_PRINT("Rumble effect hex code: 0x%lx!\n", state->effect.raw);
      enj_rumble_effect_set_raw(state->active_controller, state->effect.raw);
    }
    if (ctrl_states[state->active_controller]->button.B ==
        ENJ_BUTTON_DOWN_THIS_FRAME) {
          enj_rumble_effect_set_raw(state->active_controller, rumble_stop.raw);
      ENJ_DEBUG_PRINT("Rumble Stopped!\n");
    }
  } while (0);
  enj_render_list_add(PVR_LIST_PT_POLY, render, data);
}

int main(__unused int argc, __unused char** argv) {
  enj_state_init_defaults();
  // default soft-reset pattern is START + A + B + X + Y.
  // Lets make it easier with just START
  // START is offset 8<<1 (two bits per button)
  enj_state_soft_reset_set(ENJ_BUTTON_DOWN << (8 << 1));

  if (enj_state_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }
  RAT_state_t rat_state = {
    .catalog_index = 0,
    .cursor_pos = 0,
    .loaded_pattern = -1,
    .effect = rumble_stop
};
enj_mode_t main_mode = {
    .name = "Main Mode",
    .mode_updater = main_mode_updater,
    .data = &rat_state,
};
enj_mode_push(&main_mode);
enj_state_run();

return 0;
}
