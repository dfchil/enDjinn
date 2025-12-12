#ifndef ENJ_CTRLR_H
#define ENJ_CTRLR_H

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/vmu.h>
#include <stdint.h>

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
} enj_ctrl_port_name_e;

typedef enum {
    BUTTON_UP = 0b00,
    BUTTON_DOWN = 0b01,
    BUTTON_UP_THIS_FRAME = 0b10,
    BUTTON_DOWN_THIS_FRAME = 0b11,
} enj_button_state_e;

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
    } buttons;
    struct {
        uint32_t rumble : 1;
        uint32_t vmu : 1;
        uint32_t portnum : 4;
        uint32_t : 16;
    };
    int8_t joyx;
    int8_t joyy;
    uint8_t ltrigger;
    uint8_t rtrigger;

    cont_state_t* state;
} enj_ctrlr_state_t;

typedef struct enj_abstract_controller_s {
    struct {
        enj_controller_type_e type : 27;
        enj_ctrl_port_name_e port : 5;
    };
    void (*updatefun)(void* state, enj_ctrlr_state_t* dest);
    void* state;
} enj_abstract_ctrlr_t;

int enj_ctrl_map_states(void);

maple_device_t* enj_maple_port_type(int p, uint32 func);

enj_ctrlr_state_t** enj_ctrl_get_states(void);

void enj_read_dreamcast_controller(void* dc_ctrlr, enj_ctrlr_state_t* buttons);

void enj_read_controller(enj_abstract_ctrlr_t* ctrlref,
                         enj_ctrlr_state_t* buttons);

void enj_kos_state2ctrlrstate(cont_state_t* c_state,
                                   enj_ctrlr_state_t* ctrlrstate);

int enj_ctrlr_button_combo_raw(uint32_t raw_buttons, uint32_t combo);

int enj_ctrlr_button_combo(enj_ctrlr_state_t* cstate, enj_ctrlr_state_t* combo);

#endif  // ENJ_CTRLR_H