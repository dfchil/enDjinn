#ifndef ENJ_DEBUG_MENU_H
#define ENJ_DEBUG_MENU_H

#ifdef ENJ_DEBUG_MENU

#include <kos.h>
#include <enDjinn/enj_api.h>
#include <enDjinn/enj_ctrl.h>

ENJ_BEGIN_DECLS

typedef enum {
  ENJ_MENU_CHOICE, /**< One of a list of labels; left and right wrap around */
  ENJ_MENU_INT,    /**< An integer between min and max */
  ENJ_MENU_FLOAT,  /**< A float between min and max */
} enj_menu_row_type_e;

typedef struct {
  int *value; /**< Index into labels */
  const char *const *labels;
  int count;
} enj_menu_choice_t;

typedef struct {
  int *value;
  int min, max, step;
  const char *format; /**< printf format of the value, "%d" when NULL */
} enj_menu_int_t;

typedef struct {
  float *value;
  float min, max, step;
  const char *format; /**< printf format of the value, "%.2f" when NULL */
} enj_menu_float_t;

/**
 * One line of the menu: its name, a colon and its values.
 *
 * A NULL value pointer makes a row with no value, for on_press alone. Labels,
 * names and help texts are borrowed, not copied.
 */
typedef struct enj_menu_row_s {
  const char *name;
  const char *help; /**< Shown in the help section while the row is selected */
  enj_menu_row_type_e type;
  union {
    enj_menu_choice_t choice;
    enj_menu_int_t integer;
    enj_menu_float_t real;
  };
  /** The value changed. Called before the scene, never during a render list. */
  void (*on_change)(struct enj_menu_row_s *row, void *data);
  /** A was pressed on this row */
  void (*on_press)(struct enj_menu_row_s *row, void *data);
  /**
   * Hold on_change back until the menu closes, for a change that takes away
   * what the frame behind the menu needs: an FSAA switch frees the textures.
   */
  uint8_t apply_on_close : 1;
  uint8_t changed : 1; /**< Private: on_change is waiting for the close */
} enj_menu_row_t;

/**
 * A debug menu. Owned by the application, and it must stay alive while open.
 */
typedef struct {
  const char *title;
  const char *help; /**< Help section of a row without help of its own */
  enj_menu_row_t *rows;
  int row_count;
  /**
   * Called once per menu frame before the scene, where enj_render_list_add()
   * is allowed: queue the frozen game here. NULL leaves the background colour.
   * Anything the game must keep doing while frozen also belongs here.
   */
  void (*draw_behind)(void *data);
  void (*on_open)(void *data);
  /** Runs when the menu closes, after the held back on_change calls. */
  void (*on_close)(void *data);
  void *data; /**< Passed to every callback */

  /* Private. selected survives closing, so the menu reopens where it was. */
  int selected;
  int port;
  int repeat_dir;
  int repeat_frames;
  uint8_t closing;
} enj_debug_menu_t;

/**
 * Register the menu the button combination opens, on the mode stack.
 * @param menu Menu to open, or NULL to turn the combination off
 *
 * @note Nothing opens until this is called. The default combination is
 * Up + X + Y with both triggers held; the triggers must be down before the
 * last of the buttons is pressed.
 */
void enj_debug_menu_set(enj_debug_menu_t *menu);

/**
 * Set the buttons of the combination, in the form of
 * enj_state_soft_reset_set(). Both triggers are still required: replace
 * enj_state_get()->debug_menu_open to decide that differently.
 * @param pattern Button pattern to set
 */
void enj_debug_menu_combo_set(uint32_t pattern);

/**
 * Open the registered menu on top of the current mode, which stops updating
 * until the menu closes. Also usable as an entry in an application's own menu.
 * @param cstate The controller that drives the menu
 * @return 1 when the menu opened, 0 without a menu, when it is already open,
 * when the mode stack is full, or when the translucent list has no bins
 */
int enj_debug_menu_open(enj_ctrlr_state_t *cstate);

/**
 * Prepare a menu to be driven with enj_debug_menu_update() and
 * enj_debug_menu_draw() by an application without the mode stack. Calls
 * on_open.
 * @param menu Menu to start
 */
void enj_debug_menu_begin(enj_debug_menu_t *menu);

/**
 * Handle one frame of input. Up and down select a row, left and right change
 * its value, A presses it, B or Start close the menu.
 *
 * The menu closes once every button and trigger of the controller is released,
 * so that nothing held reaches the game as a new press. An unplugged controller
 * closes it at once. The held back on_change calls and on_close run then.
 * @param menu Menu to update
 * @param cstate The controller that drives the menu, NULL if it is gone
 * @return 1 when the menu has closed, otherwise 0
 */
int enj_debug_menu_update(enj_debug_menu_t *menu,
                          const enj_ctrlr_state_t *cstate);

/**
 * Draw the menu over whatever else is in the scene, with the quick font.
 * @param menu Menu to draw
 *
 * @note Call this last while submitting PVR_LIST_TR_POLY. The game shows through
 * the dimmed background. With autosort the sprites' z puts them on top, and
 * without it the order sent does.
 */
void enj_debug_menu_draw(enj_debug_menu_t *menu);

ENJ_END_DECLS

#endif // ENJ_DEBUG_MENU

#endif // ENJ_DEBUG_MENU_H
