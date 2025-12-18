#ifndef ENJ_CTRLR_H
#define ENJ_CTRLR_H

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/vmu.h>
#include <stdint.h>

/**
 * Controller port names,
 * @note Maple ports are labeled A-D on the Dreamcast console, ports beyond D
 * are intended for consoles linked via network or serial cable. enDjinn will
 * be expanded to support these in the future.
 */
typedef enum {
  ENJ_PORT_A = 0,
  ENJ_PORT_B = 1,
  ENJ_PORT_C = 2,
  ENJ_PORT_D = 3,
  ENJ_PORT_E = 4,
  ENJ_PORT_F = 5,
  ENJ_PORT_G = 6,
  ENJ_PORT_H = 7,
  ENJ_PORT_I = 8,
  ENJ_PORT_J = 9,
  ENJ_PORT_K = 10,
  ENJ_PORT_L = 11,
  ENJ_PORT_M = 12,
  ENJ_PORT_N = 13,
  ENJ_PORT_O = 14,
  ENJ_PORT_P = 15,
} enj_ctrl_port_name_e;

/** Using two bits per button state allows us to have one frame of permanence,
 * which makes it really easy to track button presses and releases without
 * having to involve a callback system or extra state tracking.
 */
typedef enum {
  ENJ_BUTTON_UP = 0b00,
  ENJ_BUTTON_DOWN = 0b01,
  ENJ_BUTTON_UP_THIS_FRAME = 0b10,
  ENJ_BUTTON_DOWN_THIS_FRAME = 0b11,
} enj_button_state_e;

/**
 * Controller state structure used by enDjinn, 16bytes total, or half a cache
 * line.
 */
typedef struct {
  union {
    uint32_t raw;
    struct {
      uint32_t A : 2;
      uint32_t B : 2;
      uint32_t X : 2;
      uint32_t Y : 2;
      uint32_t UP : 2;
      uint32_t DOWN : 2;
      uint32_t LEFT : 2;
      uint32_t RIGHT : 2;
      uint32_t START : 2;
      uint32_t : 14;
    };
  } button;
  struct {
    uint32_t rumble : 1;
    uint32_t vmu : 2;
    uint32_t portnum : 4;
    uint32_t : 15;
  };
  int8_t joyx;
  int8_t joyy;
  uint8_t ltrigger;
  uint8_t rtrigger;

  cont_state_t *state;
} enj_ctrlr_state_t;

/**
 * Abstract controller structure.
 *
 * @note The reasoning behind this is to allow an AI bot, a replay stream or
 * controllers linked on a remote dreamcast via network or serial to be used
 * interchangable with local maple controllers.
 */
typedef struct enj_abstract_controller_s {
  struct {
    enj_ctrl_port_name_e port : 5;
    uint32_t type : 27;
  };
  void (*updatefun)(void *state, enj_ctrlr_state_t *dest);
  void *state;
} enj_abstract_ctrlr_t;

/**
 * Get the length of the controllers state
 *
 * @return Length of the controller list, currently always MAPLE_PORT_COUNT
 *
 * @note This will be make more sense once controllers on other consoles can be
 * mapped via network or serial cable
 */
size_t enj_ctrl_states_length(void);

/**
 * Map the local maple controllers to enDjinn controller states
 *
 * @return number of mapped controllers
 *
 * @note Intended for use by the internal game loop in @ref enj_state_run()
 * @note This updates the internal enj_ctrlr_state_t structures with the
 * current state of the local maple controllers
 */
size_t enj_ctrl_map_states(void);

/** Return the first device of the requested type on port p
 * @param p Maple port number
 * @param func Maple function type to search for
 * @return Pointer to the first device of the requested type on port p, or NULL
 * if no such device is found
 */
maple_device_t *enj_maple_port_type(int p, uint32 func);

/**
 * Get the list of controller states
 * @return Pointer to an array of pointers to enj_ctrlr_state_t structures
 * representing the current state of each controller port
 * @note NULL pointers in list represent disconnected controllers
 */
enj_ctrlr_state_t **enj_ctrl_get_states(void);

/**
 * Maps the state of a locally attached Dreamcast controller to an
 * enj_ctrlr_state_t structure
 * @param dc_ctrlr Pointer to the Dreamcast controller device
 * @param state Pointer to the enj_ctrlr_state_t structure to populate
 *
 * @note Mostly intended for internal use by enDjinn
 */
void enj_read_dreamcast_controller(void *dc_ctrlr, enj_ctrlr_state_t *state);

/**
 * Read the state of an abstract controller into an enj_ctrlr_state_t structure
 * @param ctrlref Pointer to the abstract controller structure
 * @param state Pointer to the enj_ctrlr_state_t structure to populate
 *
 * @note Mostly intended for internal use by enDjinn
 */
void enj_read_controller(enj_abstract_ctrlr_t *ctrlref,
                         enj_ctrlr_state_t *state);

/**
 * Map a kos cont_state_t structure to an enj_ctrlr_state_t structure
 * @param c_state Pointer to the kos cont_state_t structure
 * @param ctrlrstate Pointer to the enj_ctrlr_state_t structure to populate
 * 
 * @note Mostly intended for internal use by enDjinn
 */
void enj_ctrl_kos2enj_state(cont_state_t *c_state,
                              enj_ctrlr_state_t *ctrlrstate);

/**
 * Check if a button combination is pressed in a raw button state with at least
 * one of the buttons pressed this frame
 * @param raw_buttons Raw button state bitfield
 * @param combo Button combination to check for
 * @return 1 if the combination is pressed, 0 otherwise
 *
 * @note The bit layout is as follows (two bits per button):
 * 0-1: A
 * 2-3: B
 * 4-5: X
 * 6-7: Y
 * 8-9: UP
 * 10-11: DOWN
 * 12-13: LEFT
 * 14-15: RIGHT
 * 16-17: START
 * 
 * @note For each button that is part of the combo variable only set the first
 * one (ie. ENJ_BUTTON_DOWN or 0b01).
 */
int enj_ctrlr_button_combo_raw(uint32_t raw_buttons, uint32_t combo);

/**
 * Check if a button combination is pressed in a controller state with at least
 * one of the buttons pressed this frame
 * @param cstate Pointer to the controller state to check
 * @param combo Pointer to the button combination to check for
 * @return 1 if the combination is pressed, 0 otherwise
 *
 * @note Set each button that are part of the combo to ENJ_BUTTON_DOWN
 * @note This is a convenience wrapper around enj_ctrlr_button_combo_raw, using
 * named buttons instead of raw bitfields
 */
int enj_ctrlr_button_combo(enj_ctrlr_state_t *cstate, enj_ctrlr_state_t *combo);

#endif // ENJ_CTRLR_H