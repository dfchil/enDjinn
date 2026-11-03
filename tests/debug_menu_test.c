#include <assert.h>
#include <enDjinn/enj_enDjinn.h>
#include <string.h>

#include "../code/enj_debug_menu.c"
#include "../code/enj_debug_menu_mode.c"

#define GLYPH_WIDTH 10

static enj_state_t test_state;
static enj_ctrlr_state_t pad;
static enj_ctrlr_state_t *ports[MAPLE_PORT_COUNT] = {&pad};
static enj_font_header_t font = {.line_height = 12};
static vid_mode_t mode_640x480 = {.width = 640, .height = 480};
vid_mode_t *vid_mode = &mode_640x480;

static float font_z = 10.0f;
static uint8_t font_scale = 3;
static char drawn[4096];
static int bands;
static int queued;

enj_state_t *enj_state_get(void) { return &test_state; }
void enj_state_flag_shutdown(void *unused) { (void)unused; }
enj_ctrlr_state_t **enj_ctrl_get_states(void) { return ports; }

int enj_ctrlr_button_combo_raw(uint32_t raw_buttons, uint32_t combo) {
  return (raw_buttons & combo) == combo && (raw_buttons & (combo << 1)) != 0;
}

enj_font_header_t *enj_qfont_get_header(void) { return &font; }
pvr_ptr_t enj_qfont_get_pvr_ptr(void) { return NULL; }
void enj_font_scale_set(uint8_t scale) { font_scale = scale; }
uint8_t enj_font_scale_get(void) { return font_scale; }
void enj_font_zvalue_set(float zvalue) { font_z = zvalue; }
float enj_font_zvalue_get(void) { return font_z; }

int enj_font_string_width(const char *text, enj_font_header_t *unused) {
  (void)unused;
  return (int)strlen(text) * GLYPH_WIDTH;
}

int enj_font_string_render(const char *text, enj_font_header_t *unused,
                           int16_t x, int16_t y, pvr_sprite_hdr_t *hdr) {
  (void)unused;
  (void)x;
  (void)y;
  (void)hdr;
  assert(font_scale == 1);
  assert(font_z == ENJ_MENU_TEXT_Z);
  strcat(drawn, text);
  strcat(drawn, "\n");
  return (int)strlen(text) * GLYPH_WIDTH;
}

void enj_draw_sprite(float corners[4][3], pvr_sprite_hdr_t *hdr,
                     uint32_t uvs[3]) {
  (void)corners;
  (void)hdr;
  (void)uvs;
  bands++;
}

void enj_render_list_add(pvr_list_t list, void (*renderer)(void *data),
                         void *data) {
  (void)list;
  (void)renderer;
  (void)data;
  queued++;
}

static int game_frames;
static int behind_frames;
static int opened;
static int closed;
static int changes;
static int deferred_changes;
static int pressed;
static int deferred_before_close;

static void game_update(void *unused) {
  (void)unused;
  game_frames++;
}

static void behind(void *unused) {
  (void)unused;
  behind_frames++;
}

static void on_open(void *unused) {
  (void)unused;
  opened++;
}

static void on_close(void *unused) {
  (void)unused;
  closed++;
  deferred_before_close = deferred_changes;
}

static void on_change(enj_menu_row_t *row, void *unused) {
  (void)row;
  (void)unused;
  changes++;
}

static void on_deferred_change(enj_menu_row_t *row, void *unused) {
  (void)row;
  (void)unused;
  deferred_changes++;
}

static void on_press(enj_menu_row_t *row, void *unused) {
  (void)row;
  (void)unused;
  pressed++;
}

static void press(uint32_t combo) { pad.button.raw = combo | (combo << 1); }
static void hold(uint32_t combo) { pad.button.raw = combo; }
static void release(void) { pad.button.raw = 0; }

/** One frame the way enj_state_run_frame does it, without the PVR. */
static void frame(void) {
  if (test_state.debug_menu_open != NULL &&
      enj_ctrlr_button_combo_raw(pad.button.raw,
                                 test_state.debug_menu_pattern)) {
    test_state.debug_menu_open(&pad);
  }
  enj_mode_t *mode = enj_mode_get();
  mode->mode_updater(mode->data);
}

static uint32_t button(int shift) { return 1u << (shift * 2); }
#define A button(0)
#define B button(1)
#define X button(2)
#define Y button(3)
#define UP button(4)
#define DOWN button(5)
#define LEFT button(6)
#define RIGHT button(7)
#define START button(8)

static int color;
static int count = 5;
static float speed = 0.5f;
static int fsaa;
static const char *const colors[] = {"Red", "Green", "Blue"};
static const char *const fsaa_labels[] = {"Off", "On"};

static enj_menu_row_t rows[] = {
    {.name = "Color",
     .help = "The colour of it all",
     .type = ENJ_MENU_CHOICE,
     .choice = {.value = &color, .labels = colors, .count = 3},
     .on_change = on_change},
    {.name = "Count",
     .type = ENJ_MENU_INT,
     .integer = {.value = &count, .min = 1, .max = 6, .step = 1},
     .on_change = on_change},
    {.name = "Speed",
     .type = ENJ_MENU_FLOAT,
     .real = {.value = &speed, .min = 0.0f, .max = 1.0f, .step = 0.25f},
     .on_change = on_change},
    {.name = "FSAA",
     .type = ENJ_MENU_CHOICE,
     .choice = {.value = &fsaa, .labels = fsaa_labels, .count = 2},
     .on_change = on_deferred_change,
     .apply_on_close = 1},
    {.name = "Reset", .type = ENJ_MENU_INT, .on_press = on_press},
};

static enj_debug_menu_t menu = {
    .title = "Debug",
    .help = "General help",
    .rows = rows,
    .row_count = 5,
    .draw_behind = behind,
    .on_open = on_open,
    .on_close = on_close,
};

static void open_menu(void) {
  pad.ltrigger = 255;
  pad.rtrigger = 255;
  press(UP | X | Y);
  frame();
  assert(enj_mode_get() == &menu_mode);
  pad.ltrigger = 0;
  pad.rtrigger = 0;
  release();
  frame();
}

static void close_menu(void) {
  press(B);
  frame();
  release();
  frame();
  assert(enj_mode_get() != &menu_mode);
}

static void test_combination(void) {
  uint32_t combo = UP | X | Y;

  enj_debug_menu_combo_set(combo);
  press(combo);
  pad.ltrigger = 255;
  pad.rtrigger = 0;
  frame();
  assert(enj_mode_get() != &menu_mode);
  pad.ltrigger = 0;
  pad.rtrigger = 255;
  frame();
  assert(enj_mode_get() != &menu_mode);

  // A trigger pulled last presses no button: the buttons must come last.
  hold(combo);
  pad.ltrigger = 255;
  pad.rtrigger = 255;
  frame();
  assert(enj_mode_get() != &menu_mode);
  int game_ran = game_frames;

  press(combo);
  frame();
  assert(enj_mode_get() == &menu_mode);
  assert(opened == 1);
  assert(game_frames == game_ran);
  // The press of Up that completed the combination did not move the cursor.
  assert(pad.button.raw == combo);
  assert(menu.selected == 0);
  assert(behind_frames == 1);
  assert(queued == 1);

  // Pressing it again does not open a menu on top of the menu.
  press(combo);
  frame();
  assert(enj_mode_get_current_index() == 1);
  assert(opened == 1);

  pad.ltrigger = 0;
  pad.rtrigger = 0;
  close_menu();
}

static void test_values(void) {
  menu.selected = 0;
  open_menu();
  int game_ran = game_frames;

  press(UP);
  frame();
  assert(menu.selected == 4);
  press(DOWN);
  frame();
  assert(menu.selected == 0);

  press(LEFT);
  frame();
  assert(color == 2);
  assert(changes == 1);
  press(RIGHT);
  frame();
  assert(color == 0);
  assert(changes == 2);
  release();
  frame();

  press(DOWN);
  frame();
  press(RIGHT);
  frame();
  assert(count == 6);
  release();
  frame();
  press(RIGHT);
  frame();
  assert(count == 6);
  assert(changes == 3);
  release();
  frame();

  press(DOWN);
  frame();
  press(LEFT);
  frame();
  assert(speed == 0.25f);
  assert(changes == 4);
  release();
  frame();

  press(DOWN);
  frame();
  press(RIGHT);
  frame();
  assert(fsaa == 1);
  assert(deferred_changes == 0);
  release();
  frame();

  press(DOWN);
  frame();
  release();
  frame();
  press(RIGHT);
  frame();
  press(A);
  frame();
  assert(pressed == 1);
  release();
  frame();

  assert(game_frames == game_ran);
  assert(deferred_changes == 0);
  press(DOWN);
  frame();
  release();
  frame();
  int closes = closed;

  // B is held: the game must not get it as a new press.
  press(B);
  frame();
  hold(B);
  for (int i = 0; i < 30; i++) {
    frame();
  }
  assert(enj_mode_get() == &menu_mode);
  assert(game_frames == game_ran);
  assert(closed == closes);

  pad.rtrigger = 255;
  release();
  frame();
  assert(enj_mode_get() == &menu_mode);

  pad.rtrigger = 0;
  frame();
  assert(enj_mode_get() != &menu_mode);
  assert(deferred_changes == 1);
  assert(deferred_before_close == 1);
  assert(closed == closes + 1);
  assert(game_frames == game_ran + 1);
}

static void test_repeat(void) {
  menu.selected = 2;
  speed = 0.0f;
  rows[2].real.step = 0.0625f;
  open_menu();

  press(RIGHT);
  frame();
  assert(speed == 0.0625f);
  hold(RIGHT);
  for (int i = 0; i < ENJ_MENU_REPEAT_DELAY - 1; i++) {
    frame();
  }
  assert(speed == 0.0625f);
  frame();
  assert(speed == 0.125f);
  for (int i = 0; i < ENJ_MENU_REPEAT_FRAMES; i++) {
    frame();
  }
  assert(speed == 0.1875f);

  // A button held from before the menu opened has not been pressed.
  release();
  frame();
  close_menu();
  pad.ltrigger = 255;
  pad.rtrigger = 255;
  press(UP | X | Y);
  frame();
  hold(RIGHT);
  for (int i = 0; i < 2 * ENJ_MENU_REPEAT_DELAY; i++) {
    frame();
  }
  assert(speed == 0.1875f);
  pad.ltrigger = 0;
  pad.rtrigger = 0;
  release();
  frame();
  close_menu();
}

static void test_unplugged(void) {
  open_menu();
  int closes = closed;
  ports[0] = NULL;
  frame();
  ports[0] = &pad;
  assert(enj_mode_get() != &menu_mode);
  assert(closed == closes + 1);
}

static void test_soft_reset_reaches_the_game(void) {
  uint32_t chord = START | A | B | X | Y;

  test_state.exit_pattern = chord;
  test_state.flags.soft_reset_enabled = 1;
  open_menu();
  int closes = closed;
  int game_ran = game_frames;

  // Held, as the host backends do to quit. It closes the menu at once, and the
  // buttons are forgotten: reading them next frame is a press for the game.
  press(chord);
  frame();
  assert(enj_mode_get() != &menu_mode);
  assert(closed == closes + 1);
  assert(game_frames == game_ran + 1);
  assert(pad.button.raw == 0);

  test_state.flags.soft_reset_enabled = 0;
  open_menu();
  press(chord);
  frame();
  assert(enj_mode_get() == &menu_mode);
  assert(menu.closing == 1);
  release();
  frame();
  assert(enj_mode_get() != &menu_mode);
}

static void test_no_translucent_bins(void) {
  test_state.video.pvr_params.opb_sizes[PVR_LIST_TR_POLY] = PVR_BINSIZE_0;
  int opens = opened;
  assert(enj_debug_menu_open(&pad) == 0);
  assert(enj_mode_get() != &menu_mode);
  assert(opened == opens);
  test_state.video.pvr_params.opb_sizes[PVR_LIST_TR_POLY] = PVR_BINSIZE_16;
}

static void test_full_stack(void) {
  enj_mode_t filler = {.mode_updater = game_update};
  int index = enj_mode_get_current_index();
  while (enj_mode_push(&filler)) {
  }
  int opens = opened;
  assert(enj_debug_menu_open(&pad) == 0);
  assert(opened == opens);
  enj_mode_goto_index(index);
}

static void test_draw(void) {
  menu.selected = 0;
  color = 1;
  font_z = 10.0f;
  font_scale = 3;
  drawn[0] = '\0';
  bands = 0;
  enj_debug_menu_draw(&menu);
  assert(font_z == 10.0f);
  assert(font_scale == 3);
  assert(bands == 2);
  assert(strstr(drawn, "Debug\n") != NULL);
  assert(strstr(drawn, "Color:\nRed\n | \nGreen\n | \nBlue\n") != NULL);
  assert(strstr(drawn, "Count:\n< 6 >\n") != NULL);
  assert(strstr(drawn, "The colour of it all\n") != NULL);
  assert(strstr(drawn, "General help") == NULL);

  // The label does not fit on the line: only the current one is shown.
  static const char *const wide[] = {
      "A label that takes up nearly the whole of the line",
      "And a second one of about the same size as that",
  };
  static enj_menu_row_t wide_row = {
      .name = "Wide",
      .type = ENJ_MENU_CHOICE,
      .choice = {.value = &color, .labels = wide, .count = 2},
  };
  enj_debug_menu_t wide_menu = {.help = "General help",
                                .rows = &wide_row,
                                .row_count = 1};
  drawn[0] = '\0';
  enj_debug_menu_draw(&wide_menu);
  assert(strstr(drawn, "< And a second one of about the same size as that >\n") !=
         NULL);
  assert(strstr(drawn, "General help\n") != NULL);

  // Enough rows to scroll keeps the selection on the screen.
  static enj_menu_row_t many[64];
  static char names[64][8];
  for (int i = 0; i < 64; i++) {
    snprintf(names[i], sizeof(names[i]), "R%d", i);
    many[i].name = names[i];
    many[i].type = ENJ_MENU_INT;
  }
  enj_debug_menu_t long_menu = {.rows = many, .row_count = 64, .selected = 60};
  drawn[0] = '\0';
  enj_debug_menu_draw(&long_menu);
  assert(strstr(drawn, ">\nR60:\n") != NULL);
  assert(strstr(drawn, "R0:") == NULL);
}

static void test_wrap(void) {
  char line[96];
  const char *rest = enj_menu_wrap("one two three", 75, line, sizeof(line));
  assert(strcmp(line, "one two") == 0);
  assert(strcmp(rest, "three") == 0);

  rest = enj_menu_wrap("one\ntwo", 200, line, sizeof(line));
  assert(strcmp(line, "one") == 0);
  assert(strcmp(rest, "two") == 0);

  rest = enj_menu_wrap("unbreakable", 50, line, sizeof(line));
  assert(strcmp(line, "unbre") == 0);
  assert(strcmp(rest, "akable") == 0);

  rest = enj_menu_wrap("all of it", 200, line, sizeof(line));
  assert(strcmp(line, "all of it") == 0);
  assert(*rest == '\0');
}

int main(void) {
  enj_mode_t game = {.name = "game", .mode_updater = game_update};

  assert(enj_mode_push(&game) == 1);
  test_state.debug_menu_pattern = UP | X | Y;
  test_state.video.pvr_params.opb_sizes[PVR_LIST_TR_POLY] = PVR_BINSIZE_16;
  assert(enj_debug_menu_open(&pad) == 0);

  enj_debug_menu_set(&menu);
  test_combination();
  test_values();
  test_repeat();
  test_unplugged();
  test_soft_reset_reaches_the_game();
  test_no_translucent_bins();
  test_full_stack();
  test_draw();
  test_wrap();

  enj_debug_menu_set(NULL);
  assert(test_state.debug_menu_open == NULL);
  assert(enj_debug_menu_open(&pad) == 0);
  return 0;
}
