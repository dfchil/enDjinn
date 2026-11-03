#include <enDjinn/enj_enDjinn.h>

#define SQUARE_SIZE 32.0f

static const uint32_t square_colors[] = {0xffe04040, 0xff40e040, 0xff4080ff};
static const char *const color_names[] = {"Red", "Green", "Blue"};
static const char *const background_names[] = {"Black", "Navy"};

static float position;
static float speed = 2.0f;
static int squares = 5;
static int color = 2;
static int background;

static void draw_squares(void *__unused) {
  pvr_sprite_cxt_t cxt;
  pvr_sprite_hdr_t hdr;

  pvr_sprite_cxt_col(&cxt, PVR_LIST_OP_POLY);
  pvr_sprite_compile(&hdr, &cxt);
  hdr.argb = square_colors[color];
  for (int i = 0; i < squares; i++) {
    float min_x = (int)(position + i * 60.0f) % 600;
    float max_x = min_x + SQUARE_SIZE;
    float corners[4][3] = {{min_x * ENJ_XSCALE, 240.0f, 1.0f},
                           {min_x * ENJ_XSCALE, 240.0f - SQUARE_SIZE, 1.0f},
                           {max_x * ENJ_XSCALE, 240.0f - SQUARE_SIZE, 1.0f},
                           {max_x * ENJ_XSCALE, 240.0f, 1.0f}};
    enj_draw_sprite(corners, i == 0 ? &hdr : NULL, NULL);
  }
}

static void draw_hint(void *__unused) {
  enj_qfont_write("Hold L+R and press Up+X+Y for the debug menu.", 20, 20,
                  PVR_LIST_PT_POLY);
}

// The menu freezes the game by not calling main_mode_updater. It draws what the
// game last showed by queueing the same renderers, without moving anything.
static void queue_game(void *__unused) {
  enj_render_list_add(PVR_LIST_OP_POLY, draw_squares, NULL);
  enj_render_list_add(PVR_LIST_PT_POLY, draw_hint, NULL);
}

static void main_mode_updater(void *__unused) {
  position += speed;
  queue_game(NULL);
}

static void apply_background(__unused enj_menu_row_t *row,
                             __unused void *data) {
  pvr_set_bg_color(0.0f, 0.0f, background ? 0.25f : 0.0f);
}

static void restart(__unused enj_menu_row_t *row, __unused void *data) {
  position = 0;
}

static enj_menu_row_t rows[] = {
    {.name = "Speed",
     .help = "A float value. Hold left or right to keep changing it.",
     .type = ENJ_MENU_FLOAT,
     .real = {.value = &speed, .min = 0.0f, .max = 8.0f, .step = 0.25f}},
    {.name = "Squares",
     .help = "An integer value.",
     .type = ENJ_MENU_INT,
     .integer = {.value = &squares, .min = 1, .max = 10, .step = 1}},
    {.name = "Color",
     .help = "A choice of labels. The game behind the menu changes with it.",
     .type = ENJ_MENU_CHOICE,
     .choice = {.value = &color, .labels = color_names, .count = 3}},
    {.name = "Background",
     .help = "This one is held back until the menu closes (apply_on_close), "
             "which is what a change that frees the game's textures needs.",
     .type = ENJ_MENU_CHOICE,
     .choice = {.value = &background, .labels = background_names, .count = 2},
     .on_change = apply_background,
     .apply_on_close = 1},
    {.name = "Restart",
     .help = "A row with no value: press A.",
     .on_press = restart},
};

static enj_debug_menu_t menu = {
    .title = "enDjinn debug menu",
    .rows = rows,
    .row_count = sizeof(rows) / sizeof(rows[0]),
    .draw_behind = queue_game,
};

int main(__unused int argc, __unused char **argv) {
  enj_state_init_defaults();
  if (enj_state_startup() != 0) {
    ENJ_DEBUG_PRINT("enDjinn startup failed, exiting\n");
    return -1;
  }
  enj_mode_t main_mode = {
      .name = "Main Mode",
      .mode_updater = main_mode_updater,
  };
  enj_mode_push(&main_mode);
  enj_debug_menu_set(&menu);
  enj_state_run();
  return 0;
}
