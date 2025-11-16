#ifndef ENDJINN_CONTROLLER_CORE_H
#define ENDJINN_CONTROLLER_CORE_H

#include <stdint.h>
#include <dc/maple/controller.h>

typedef enum {
  PORT_A = 0,
  PORT_B = 1,
  PORT_C = 2,
  PORT_D = 3,
  PORT_E = 4,
  PORT_F = 5,
  PORT_G = 6,
  PORT_H = 7,
} port_name_e;

typedef enum {
  BUTTON_UP = 0b00,
  BUTTON_DOWN = 0b01,
  BUTTON_UP_THIS_FRAME = 0b10,
  BUTTON_DOWN_THIS_FRAME = 0b11,
} button_state_e;

typedef struct {
  union {
    uint8_t ABXY;
    struct {
      uint8_t BTN_A : 2;
      uint8_t BTN_B : 2;
      uint8_t BTN_X : 2;
      uint8_t BTN_Y : 2;
    };
  };
  union {
    uint8_t dpad;
    struct {
      uint8_t UP : 2;
      uint8_t DOWN : 2;
      uint8_t LEFT : 2;
      uint8_t RIGHT : 2;
    };
  };
  int8_t joyx;
  int8_t joyy;
  uint8_t ltrigger;
  uint8_t rtrigger;
  struct {
    uint16_t START : 2;
    uint16_t rumble : 1;
    uint16_t vmu : 1;
    uint16_t portnum : 4;
    uint16_t : 8;
  };

  cont_state_t *state;
} controller_state_t;


#endif // ENDJINN_CONTROLLER_CORE_H