#ifndef ENJ_CTRLR_H
#define ENJ_CTRLR_H

#include <stdint.h>
#include <dc/maple/controller.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/vmu.h>

typedef enum {
  PORT_A = 0,
  PORT_B = 1,
  PORT_C = 2,
  PORT_D = 3,
  PORT_E = 4,
  PORT_F = 5,
  PORT_G = 6,
  PORT_H = 7,
  PORT_I = 8,
  PORT_J = 9,
  PORT_K = 10,
  PORT_L = 11,
  PORT_M = 12,
  PORT_N = 13,
  PORT_O = 14,
  PORT_P = 15,  
} port_name_e;

typedef enum {
  BUTTON_UP = 0b00,
  BUTTON_DOWN = 0b01,
  BUTTON_UP_THIS_FRAME = 0b10,
  BUTTON_DOWN_THIS_FRAME = 0b11,
} button_state_e;

// TODO: probably unecessary, remove in future
typedef enum controller_type {
  NO_CONTROLLER_E = 0,
  DREAMCAST_STD_E = 1,
  SERIAL_CABLE_E = 2,
  NETWORK_E = 3,
  REPLAY_E = 4,
  AIBOT_E = 5,
} enj_controller_type_e;



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
  struct {
    uint16_t BTN_START : 2;
    uint16_t rumble : 1;
    uint16_t vmu : 1;
    uint16_t portnum : 4;
    uint16_t : 8;
  };
  int8_t joyx;
  int8_t joyy;
  uint8_t ltrigger;
  uint8_t rtrigger;

  cont_state_t *state;
} enj_ctrlr_state_t;

typedef void (*enj_ctrl_state_updater)(void *state, enj_ctrlr_state_t* dest);

typedef struct enj_abstract_controller_s {
  struct {
    enj_controller_type_e type : 27;
    port_name_e port : 5;
  };
  enj_ctrl_state_updater updatefun;
  void *state;
} enj_abstract_ctrlr_t;

int enj_dc_ctrlrs_map_state(void);
maple_device_t *enj_maple_port_type(int p, uint32 func);
enj_ctrlr_state_t **enj_get_ctrlr_states(void);

void enj_read_dreamcast_controller(void *dc_ctrlr,
                               enj_ctrlr_state_t *buttons);


void enj_read_controller(enj_abstract_ctrlr_t *ctrlref,
                     enj_ctrlr_state_t *buttons);

void enj_cont_state_onto_ctrlstate(cont_state_t *c_state,
                               enj_ctrlr_state_t *ctrlr);



#endif // ENJ_CTRLR_H