#ifdef ENJ_DEBUG_MENU

#ifndef ENJ_INJECT_QFONT
#error "ENJ_DEBUG_MENU draws with the quick font: define ENJ_INJECT_QFONT"
#endif

#include <enDjinn/enj_debug_menu.h>
#include <enDjinn/enj_draw.h>
#include <enDjinn/enj_font.h>
#include <enDjinn/enj_qfont.h>
#include <stdio.h>
#include <string.h>

#define ENJ_MENU_HELD_MASK 0x55555555u // the "held now" bit of every button
#define ENJ_MENU_TRIGGER_DOWN 128
#define ENJ_MENU_REPEAT_DELAY 20
#define ENJ_MENU_REPEAT_FRAMES 4

#define ENJ_MENU_MARGIN_X 48
#define ENJ_MENU_MARGIN_Y 36
#define ENJ_MENU_HELP_LINES 3
#define ENJ_MENU_LINE_GAP 3

// Above anything a game draws, and in the order they cover each other: the
// translucent list sorts by z unless the application turned autosort off.
#define ENJ_MENU_DIM_Z 100000.0f
#define ENJ_MENU_HELP_Z 110000.0f
#define ENJ_MENU_TEXT_Z 120000.0f

#define ENJ_MENU_DIM_ARGB 0xa0000000u
#define ENJ_MENU_HELP_ARGB 0xc0000030u
#define ENJ_MENU_COLOR_TITLE 0xffffffffu
#define ENJ_MENU_COLOR_NAME 0xffb8b8b8u
#define ENJ_MENU_COLOR_SELECTED 0xffffe040u
#define ENJ_MENU_COLOR_VALUE 0xffffffffu
#define ENJ_MENU_COLOR_OTHER 0xff707070u
#define ENJ_MENU_COLOR_HELP 0xffa0d0ffu

static pvr_sprite_hdr_t enj_menu_text_hdr;

static int enj_menu_held(int button) { return (button & ENJ_BUTTON_DOWN) != 0; }

static int enj_menu_released(const enj_ctrlr_state_t *cstate) {
  return (cstate->button.raw & ENJ_MENU_HELD_MASK) == 0 &&
         cstate->ltrigger < ENJ_MENU_TRIGGER_DOWN &&
         cstate->rtrigger < ENJ_MENU_TRIGGER_DOWN;
}

/** Left or right as -1 or 1 when they count this frame: on the press itself,
 * and repeating while it is held. A button already held when the menu opened
 * never pressed, so it does not repeat either. */
static int enj_menu_direction(enj_debug_menu_t *menu,
                              const enj_ctrlr_state_t *cstate) {
  int dir = enj_menu_held(cstate->button.RIGHT) -
            enj_menu_held(cstate->button.LEFT);

  if (cstate->button.LEFT == ENJ_BUTTON_DOWN_THIS_FRAME ||
      cstate->button.RIGHT == ENJ_BUTTON_DOWN_THIS_FRAME) {
    menu->repeat_dir = dir;
    menu->repeat_frames = 0;
    return dir;
  }
  if (dir == 0 || dir != menu->repeat_dir) {
    menu->repeat_dir = 0;
    return 0;
  }
  menu->repeat_frames++;
  if (menu->repeat_frames < ENJ_MENU_REPEAT_DELAY) {
    return 0;
  }
  return (menu->repeat_frames - ENJ_MENU_REPEAT_DELAY) %
                 ENJ_MENU_REPEAT_FRAMES ==
             0
             ? dir
             : 0;
}

/** @return 1 when the row's value changed */
static int enj_menu_step(enj_menu_row_t *row, int dir) {
  switch (row->type) {
  case ENJ_MENU_CHOICE: {
    int count = row->choice.count;
    int *value = row->choice.value;
    if (value == NULL || count < 1) {
      return 0;
    }
    int next = ((*value + dir) % count + count) % count;
    if (next == *value) {
      return 0;
    }
    *value = next;
    return 1;
  }
  case ENJ_MENU_INT: {
    int *value = row->integer.value;
    if (value == NULL) {
      return 0;
    }
    int next = *value + dir * row->integer.step;
    next = next < row->integer.min ? row->integer.min : next;
    next = next > row->integer.max ? row->integer.max : next;
    if (next == *value) {
      return 0;
    }
    *value = next;
    return 1;
  }
  case ENJ_MENU_FLOAT: {
    float *value = row->real.value;
    if (value == NULL) {
      return 0;
    }
    float next = *value + dir * row->real.step;
    next = next < row->real.min ? row->real.min : next;
    next = next > row->real.max ? row->real.max : next;
    if (next == *value) {
      return 0;
    }
    *value = next;
    return 1;
  }
  }
  return 0;
}

static void enj_menu_finish(enj_debug_menu_t *menu) {
  for (int i = 0; i < menu->row_count; i++) {
    enj_menu_row_t *row = &menu->rows[i];
    if (row->changed) {
      row->changed = 0;
      if (row->on_change != NULL) {
        row->on_change(row, menu->data);
      }
    }
  }
  if (menu->on_close != NULL) {
    menu->on_close(menu->data);
  }
}

void enj_debug_menu_begin(enj_debug_menu_t *menu) {
  if (menu == NULL) {
    return;
  }
  menu->closing = 0;
  menu->repeat_dir = 0;
  menu->repeat_frames = 0;
  if (menu->selected < 0 || menu->selected >= menu->row_count) {
    menu->selected = 0;
  }
  if (menu->on_open != NULL) {
    menu->on_open(menu->data);
  }
}

int enj_debug_menu_update(enj_debug_menu_t *menu,
                          const enj_ctrlr_state_t *cstate) {
  if (menu == NULL) {
    return 1;
  }
  if (cstate == NULL) {
    enj_menu_finish(menu);
    return 1;
  }
  if (menu->closing) {
    if (!enj_menu_released(cstate)) {
      return 0;
    }
    enj_menu_finish(menu);
    return 1;
  }
  if (cstate->button.B == ENJ_BUTTON_DOWN_THIS_FRAME ||
      cstate->button.START == ENJ_BUTTON_DOWN_THIS_FRAME) {
    menu->closing = 1;
    return 0;
  }
  if (menu->row_count < 1) {
    return 0;
  }
  if (cstate->button.UP == ENJ_BUTTON_DOWN_THIS_FRAME) {
    menu->selected = (menu->selected + menu->row_count - 1) % menu->row_count;
  }
  if (cstate->button.DOWN == ENJ_BUTTON_DOWN_THIS_FRAME) {
    menu->selected = (menu->selected + 1) % menu->row_count;
  }

  enj_menu_row_t *row = &menu->rows[menu->selected];
  int dir = enj_menu_direction(menu, cstate);
  if (dir != 0 && enj_menu_step(row, dir)) {
    if (row->apply_on_close) {
      row->changed = 1;
    } else if (row->on_change != NULL) {
      row->on_change(row, menu->data);
    }
  }
  if (cstate->button.A == ENJ_BUTTON_DOWN_THIS_FRAME && row->on_press != NULL) {
    row->on_press(row, menu->data);
  }
  return 0;
}

static int enj_menu_text(const char *text, int x, int y, uint32_t argb) {
  enj_menu_text_hdr.argb = argb;
  return enj_font_string_render(text, enj_qfont_get_header(), (int16_t)x,
                                (int16_t)y, &enj_menu_text_hdr);
}

/** Dim a band of the screen. It runs twice the screen's width, which covers
 * an FSAA surface too; the PVR clips what is past the edge. */
static void enj_menu_band(float top, float bottom, uint32_t argb, float z) {
  pvr_sprite_cxt_t cxt;
  pvr_sprite_hdr_t hdr;
  float right = vid_mode->width * 2.0f;
  float corners[4][3] = {{0.0f, bottom, z},
                         {0.0f, top, z},
                         {right, top, z},
                         {right, bottom, z}};

  pvr_sprite_cxt_col(&cxt, PVR_LIST_TR_POLY);
  pvr_sprite_compile(&hdr, &cxt);
  hdr.argb = argb;
  enj_draw_sprite(corners, &hdr, NULL);
}

static int enj_menu_choices_width(const enj_menu_choice_t *choice) {
  enj_font_header_t *font = enj_qfont_get_header();
  int width = 0;

  for (int i = 0; i < choice->count; i++) {
    width += enj_font_string_width(choice->labels[i], font);
    if (i > 0) {
      width += enj_font_string_width(" | ", font);
    }
  }
  return width;
}

/** Print the row's value when it is a single one. @return 1 when there is one */
static int enj_menu_format_value(const enj_menu_row_t *row, char *text,
                                 size_t size) {
  switch (row->type) {
  case ENJ_MENU_CHOICE: {
    const enj_menu_choice_t *choice = &row->choice;
    if (choice->value == NULL || choice->labels == NULL) {
      return 0;
    }
    int value = *choice->value;
    snprintf(text, size, "< %s >",
             value >= 0 && value < choice->count ? choice->labels[value] : "?");
    return 1;
  }
  case ENJ_MENU_INT: {
    char value[32];
    if (row->integer.value == NULL) {
      return 0;
    }
    snprintf(value, sizeof(value),
             row->integer.format != NULL ? row->integer.format : "%d",
             *row->integer.value);
    snprintf(text, size, "< %s >", value);
    return 1;
  }
  case ENJ_MENU_FLOAT: {
    char value[32];
    if (row->real.value == NULL) {
      return 0;
    }
    snprintf(value, sizeof(value),
             row->real.format != NULL ? row->real.format : "%.2f",
             (double)*row->real.value);
    snprintf(text, size, "< %s >", value);
    return 1;
  }
  }
  return 0;
}

/** Every label of a choice when the line has room for them, the rest as the
 * single current value. */
static void enj_menu_draw_values(const enj_menu_row_t *row, int x, int y,
                                 int width, uint32_t color) {
  char text[96];

  if (row->type == ENJ_MENU_CHOICE && row->choice.value != NULL &&
      row->choice.labels != NULL &&
      enj_menu_choices_width(&row->choice) <= width) {
    for (int i = 0; i < row->choice.count; i++) {
      if (i > 0) {
        x += enj_menu_text(" | ", x, y, ENJ_MENU_COLOR_OTHER);
      }
      x += enj_menu_text(row->choice.labels[i], x, y,
                         i == *row->choice.value ? color : ENJ_MENU_COLOR_OTHER);
    }
    return;
  }
  if (enj_menu_format_value(row, text, sizeof(text))) {
    enj_menu_text(text, x, y, color);
  }
}

static int enj_menu_is_break(char c) {
  return c == ' ' || c == '\n' || c == '\0';
}

/** Cut one line off text, at the last word that fits in width. A word wider
 * than the line is cut where it stops fitting.
 * @return The text after the line */
static const char *enj_menu_wrap(const char *text, int width, char *line,
                                 size_t size) {
  enj_font_header_t *font = enj_qfont_get_header();
  size_t len = 0;
  size_t fit = 0;
  int used = 0;

  while (text[len] != '\0' && text[len] != '\n' && len + 1 < size) {
    char glyph[2] = {text[len], '\0'};
    used += enj_font_string_width(glyph, font);
    if (used > width) {
      break;
    }
    len++;
    if (enj_menu_is_break(text[len])) {
      fit = len;
    }
  }
  if (fit == 0) {
    fit = len;
  }
  memcpy(line, text, fit);
  line[fit] = '\0';
  text += fit;
  return *text == ' ' || *text == '\n' ? text + 1 : text;
}

void enj_debug_menu_draw(enj_debug_menu_t *menu) {
  enj_font_header_t *font = enj_qfont_get_header();
  if (menu == NULL || font == NULL) {
    return;
  }
  char text[96];
  float font_z = enj_font_zvalue_get();
  uint8_t font_scale = enj_font_scale_get();
  pvr_sprite_cxt_t cxt;

  enj_font_scale_set(1);
  enj_font_zvalue_set(ENJ_MENU_TEXT_Z);
  // A header of its own: qfont's belongs to the frozen game, which may have
  // set a colour or another list on it.
  pvr_sprite_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555,
                     1 << font->log2width, 1 << font->log2height,
                     enj_qfont_get_pvr_ptr(), PVR_FILTER_NEAREST);
  pvr_sprite_compile(&enj_menu_text_hdr, &cxt);

  int line = font->line_height + ENJ_MENU_LINE_GAP;
  int left = ENJ_MENU_MARGIN_X;
  int right = vid_mode->width - ENJ_MENU_MARGIN_X;
  int bottom = vid_mode->height - ENJ_MENU_MARGIN_Y;
  int help_top = bottom - (ENJ_MENU_HELP_LINES + 1) * line;
  int y = ENJ_MENU_MARGIN_Y;

  enj_menu_band(0.0f, (float)vid_mode->height, ENJ_MENU_DIM_ARGB,
                ENJ_MENU_DIM_Z);
  enj_menu_band((float)(help_top - line / 2), (float)vid_mode->height,
                ENJ_MENU_HELP_ARGB, ENJ_MENU_HELP_Z);

  if (menu->title != NULL) {
    enj_menu_text(menu->title, left, y, ENJ_MENU_COLOR_TITLE);
  }
  y += line * 2;

  int name_x = left + enj_font_string_width("> ", font);
  int value_x = 0;
  for (int i = 0; i < menu->row_count; i++) {
    snprintf(text, sizeof(text), "%s: ", menu->rows[i].name);
    int width = enj_font_string_width(text, font);
    value_x = width > value_x ? width : value_x;
  }
  value_x += name_x;

  int visible = (help_top - line - y) / line;
  int first = 0;
  if (menu->row_count > visible) {
    first = menu->selected - visible / 2;
    first = first > menu->row_count - visible ? menu->row_count - visible : first;
    first = first < 0 ? 0 : first;
  }
  for (int i = first; i < menu->row_count && i < first + visible; i++) {
    const enj_menu_row_t *row = &menu->rows[i];
    int selected = i == menu->selected;

    if (selected) {
      enj_menu_text(">", left, y, ENJ_MENU_COLOR_SELECTED);
    }
    snprintf(text, sizeof(text), "%s:", row->name);
    enj_menu_text(text, name_x, y,
                  selected ? ENJ_MENU_COLOR_SELECTED : ENJ_MENU_COLOR_NAME);
    enj_menu_draw_values(row, value_x, y, right - value_x,
                         selected ? ENJ_MENU_COLOR_SELECTED
                                  : ENJ_MENU_COLOR_VALUE);
    y += line;
  }

  const char *help = menu->help;
  if (menu->row_count > 0 && menu->rows[menu->selected].help != NULL) {
    help = menu->rows[menu->selected].help;
  }
  y = help_top;
  for (int i = 0; help != NULL && *help != '\0' && i < ENJ_MENU_HELP_LINES;
       i++) {
    help = enj_menu_wrap(help, right - left, text, sizeof(text));
    enj_menu_text(text, left, y, ENJ_MENU_COLOR_HELP);
    y += line;
  }
  enj_menu_text("Up/Down: row   Left/Right: value   A: press   B: close", left,
                bottom - line, ENJ_MENU_COLOR_OTHER);

  enj_font_scale_set(font_scale);
  enj_font_zvalue_set(font_z);
}

#endif // ENJ_DEBUG_MENU
