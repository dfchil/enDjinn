#ifndef GAME_CONTROLLER_ABSTRACT_H
#define GAME_CONTROLLER_ABSTRACT_H
#include <enDJinn/shared.h>
// #include <enDJinn/entity/player.h>
// #include <enDJinn/controller/aicontroller.h>
#include <enDJinn/ai/core.h>
#include <enDJinn/controller/core.h>

/**
 * button 4 states
 * 0 - not pressed
 * 1 - pressed this frame
 * 2 - pressed continuously
 * 3 - released this frame
 */

typedef enum controller_type {
  NO_CONTROLLER_E = 0,
  DREAMCAST_STD_E = 1,
  SERIAL_CABLE_E = 2,
  NETWORK_E = 3,
  REPLAY_E = 4,
  AIBOT_E = 5,
} controller_type_e;

typedef struct {
  void *ctrl_box;
  int offset;
} replay_controller_t;

typedef struct {
  // struct {
  //   int offset : 16;
  //   uint32_t avoid_mine_frames : 8;
  //   uint32_t _ : 8;
  // };
  // // void *avoid_mine;
  // ai_goal_t goal;
  // void *source;
  uint8_t *profile;
} aibot_controller_t;

typedef struct {
  int offset;
} dc_controller_t;

typedef struct {
  struct {
    controller_type_e type : 4;
    port_name_e port : 4;
    uint32_t _ : 24;
  };
  union {
    dc_controller_t dc_ctrlr;
    replay_controller_t replay;
    aibot_controller_t aibot;
  };
} abstract_controller_t;

typedef struct {
  uint32_t current_ctrl_frame;
  uint32_t current_render_frame;
  // int status;
  int num_players;
  int total_frames;
  void *replay;
  void *events;
  void *positions;
  // abstract_controller_t controllers[8];
  int current_replay;
  uint32 serialfactory;
} ctrl_box_t;

ctrl_box_t *get_ctrl_box(void);

void control_next_frame(ctrl_box_t *ctrl_box);
void read_controller(abstract_controller_t *ctrlref,
                     controller_state_t *buttons);

void cont_state_onto_ctrlstate(cont_state_t *c_state,
                               controller_state_t *ctrlr);
#endif // GAME_CONTROLLER_ABSTRACT_H  