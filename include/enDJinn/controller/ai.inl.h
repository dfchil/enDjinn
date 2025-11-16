#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/vmu.h>
#include <dc/vec3f.h>
#include <enDJinn/compiledefs.h>
#include <enDJinn/controller/abstract_ctrlr.h>
#include <enDJinn/controller/aicontroller.h>
#include <enDJinn/controller/core.h>
#include <enDJinn/entity/player.h>
#include <enDJinn/entity/rect_grid.h>
#include <enDJinn/render/pvr/model/player.h>
#include <enDJinn/render/pvr/pvr.h>
#include <enDJinn/render/pvr/transformers/curver.h>
#include <stddef.h>

static inline void _ai_set_move2(aibot_controller_t *aictrl, vec2d_t target,
                                 float speed, int airbrake) {

  aictrl->goal.action = AI_GOAL_MOVE_TO;
  aictrl->goal.expiration_frame = 0;
  aictrl->goal.description.move_to.airbrake = airbrake;
  aictrl->goal.description.move_to.speed = MIN(1.0f, speed);

  rect_grid_t *grid = grid_get();
  if (target.x < grid->gamespace.min.x) {
    target.x = grid->gamespace.min.x;
  } else if (target.x > grid->gamespace.max.x) {
    target.x = grid->gamespace.max.x;
  }
  if (target.y < grid->gamespace.min.y) {
    target.y = grid->gamespace.min.y;
  } else if (target.y > grid->gamespace.max.y) {
    target.y = grid->gamespace.max.y;
  }
  aictrl->goal.description.move_to.target = target;
}

void ai_set_next_goal(abstract_controller_t *abc) {
  g_state_t *g_point = core_get_global();
  aibot_controller_t *aictrl = &abc->aibot;
  int r = rand() % aictrl->profile[AI_GOAL_COUNT];

  ai_goal_type_e goal = 0;
  for (int i = 0; i < AI_GOAL_COUNT; i++) {
    r -= aictrl->profile[i];
    if (r <= 0) {
      goal = i;
      break;
    }
  }

  // goal = AI_GOAL_GLIDEMINE;
  aictrl->goal.expiration_frame = 0;
  switch (goal) {
  case AI_GOAL_MOVE_TO:
    _ai_set_move2(
        aictrl, (vec2d_t){(float)(rand() % 46) - 23, (float)(rand() % 46) - 23},
        0.66f + ((float)(rand() % 255) / 255.0f), 1);
    aictrl->goal.expiration_frame = g_point->round_time + 300;
    break;
  case AI_GOAL_JUIZE2SHIELD:
    aictrl->goal.action = AI_GOAL_JUIZE2SHIELD;
    aictrl->goal.description.juize2shield.health_pct =
        0.5f + (float)(rand() % 1000) / 2000.0f;
    aictrl->goal.expiration_frame = g_point->round_time + 300 + (rand() % 600);

    break;
  case AI_GOAL_GETJUIZED:
    aictrl->goal.action = AI_GOAL_GETJUIZED;
    aictrl->goal.description.charge.juize_to = COST_MINE * (rand() % 10);
    break;
  case AI_GOAL_MINES:
    aictrl->goal.action = AI_GOAL_MINES;
    aictrl->goal.description.mine.num_mines = 1 + rand() % 4;
    break;
  case AI_GOAL_SHOOT:
    aictrl->goal.action = AI_GOAL_SHOOT;
    aictrl->goal.expiration_frame = g_point->round_time + 300;
    aictrl->goal.description.shoot.num_shots = 1 + rand() % 6;
    aictrl->goal.description.shoot.target = g_point->first_player;
    break;
  case AI_GOAL_GLIDEMINE:
    aictrl->goal.action = AI_GOAL_GLIDEMINE;
    aictrl->goal.description.glidebomb.num_bombs = 1;
    aictrl->goal.description.glidebomb.target = g_point->first_player;
    break;
  case AI_GOAL_COUNT:
  default:
    break;
  }
}

static int _move_towards(abstract_controller_t *abc, controller_state_t *ctrlr,
                         vec2d_t *trgtp, float speed, int airbrake,
                         float *rot_diff) {
  aibot_controller_t *aictrl = &abc->aibot;

  player_t *p = (player_t *)aictrl->source;
  vec2d_t *srcp = &(p->position);
  vec3f_t dist = {trgtp->x - srcp->x, trgtp->y - srcp->y, 0.0f};
  float dstsq = dist.x * dist.x + dist.y * dist.y;
  if (dstsq < 4.0f) {
    return 1;
  }
  float rotation = atan2f(dist.x, dist.y + (dist.y == 0.0f ? 0.001f : 0.0f));
  if (rotation < 0.0f * F_PI) {
    rotation += 2.0f * F_PI;
  }
  float speedfac =
      1.0f - MIN(F_PI * 0.5, fabsf(p->orientation - rotation)) / (0.5f * F_PI);
  if (rot_diff != NULL) {
    *rot_diff = speedfac;
  }
  vec3f_normalize(dist.x, dist.y, dist.z);
  ctrlr->joyx = dist.x * 127;
  ctrlr->joyy = -dist.y * 127;
  ctrlr->rtrigger = speed * speedfac * 255;

  if (airbrake) {
    speedfac = 1.0f - speedfac;
    if (speedfac < 0.5f) {
      speedfac = 0.0f;
    }
    // if (p->speed > (0.44 * 1.0f/13.0f)){
    speedfac = speedfac * speedfac * speedfac * speedfac;
    ctrlr->ltrigger = MAX(0.1f, speedfac * 255);
    // }
  }

  // old_player_model_allocation_t *pmodel = (old_player_model_allocation_t *)p;
  // model_state_t p_poss;
  // p_poss.location = *trgtp;
  // p_poss.model = &pmodel->trpz_hdr;
  // p_poss.rotation = rotation;
  // add_to_renderlist(PVR_LIST_OP_POLY, render_generic_2d_model, &p_poss,
  //                   get_vertex_curver());

  return 0;
}

// static inline void _avoid_nearest_mines(abstract_controller_t *abc,
//                                         controller_state_t *ctrlr) {
//   aibot_controller_t *aictrl = &abc->aibot;
//   player_t *p = (player_t *)aictrl->source;
//   vec2d_t srcp = p->position;
//   mine_t *mine = core_get_global()->first_mine;
//   int count_near = 0;
//   vec3d_t sum_avoid = {0.0f, 0.0f, 0.0f};

//   while (mine) {
//     // if (mine->source == p && mine->countdown > MINE_COUNTDOWN - 30) {
//     //   mine = mine->next;
//     //   continue;
//     // }
//     float dx = srcp.x - mine->position.x;
//     float dy = srcp.y - mine->position.y;
//     float dstsq = dx * dx + dy * dy;
//     if (dstsq < (6.0f * 6.0f)) {
//       count_near++;
//       sum_avoid.x += dx / dstsq;
//       sum_avoid.y += dy / dstsq;
//     }
//     mine = mine->next;
//   }
//   if (count_near == 0) {
//     return;
//   }
//   vec3f_normalize(sum_avoid.x, sum_avoid.y, sum_avoid.z);
//   vec2d_t moveto = {4.0f * (sum_avoid.x + p->position.x),
//                     4.0f * (sum_avoid.y + p->position.y)};
//   _move_towards(abc, ctrlr, &moveto, 1.0f, 0, NULL);
// }

// static inline mine_t *_nearest_mine(abstract_controller_t *abc) {
//   aibot_controller_t *aictrl = &abc->aibot;
//   player_t *bot = (player_t *)aictrl->source;
//   vec2d_t bpos = (bot->position);
//   mine_t *mine = core_get_global()->first_mine;
//   mine_t *nearest = NULL;
//   float mindstsq = 1000000.0f;
//   while (mine) {
//     if (mine->source == bot && mine->countdown > MINE_COUNTDOWN - 10) {
//       mine = mine->next;
//       continue;
//     }
//     float dstsq = (mine->position.x - bpos.x) * (mine->position.x - bpos.x) +
//                   (mine->position.y - bpos.y) * (mine->position.y - bpos.y);
//     if (dstsq < mindstsq) {
//       mindstsq = dstsq;
//       nearest = mine;
//     }
//     mine = mine->next;
//   }
//   if (nearest == NULL || mindstsq > (6.0f * 6.0f)) {
//     return NULL;
//   }
//   return nearest;
// }

static inline void _avoid_nearest_mine(abstract_controller_t *abc,
                                       controller_state_t *ctrlr) {
  aibot_controller_t *aictrl = &abc->aibot;
  player_t *p = (player_t *)aictrl->source;
  vec2d_t *srcp = &(p->position);
  float dstsq = 0.0f;
  float mindstsq = 1000000.0f;
  mine_t *mine = core_get_global()->first_mine;
  mine_t *nearest = NULL;
  while (mine) {
    if (mine->source == p && mine->countdown > MINE_COUNTDOWN - 30) {
      mine = mine->next;
      continue;
    }
    dstsq = (mine->position.x - srcp->x) * (mine->position.x - srcp->x) +
            (mine->position.y - srcp->y) * (mine->position.y - srcp->y);
    if (dstsq < mindstsq) {
      mindstsq = dstsq;
      nearest = mine;
    }
    mine = mine->next;
  }
  if (nearest == NULL || mindstsq > (6.0f * 6.0f)) {
    return;
  }
  vec2d_t moveto = {(2.0f * p->position.x - nearest->position.x + 0.1f),
                    (2.0f * p->position.y - nearest->position.y + 0.1f)};

  _move_towards(abc, ctrlr, &moveto, 1.0f, 0, NULL);
  // _ai_set_move2(aictrl, moveto, 1.0f, 1);
  // aictrl->goal.expiration_frame = core_get_global()->round_time + 16;
}

void read_aibot_controller(abstract_controller_t *abc,
                           controller_state_t *ctrlr) {

  aibot_controller_t *aictrl = &abc->aibot;
  g_state_t *g_point = core_get_global();

  ctrlr->BTN_A = BUTTON_UP;
  ctrlr->BTN_B = BUTTON_UP;
  ctrlr->BTN_X = BUTTON_UP;
  ctrlr->BTN_Y = BUTTON_UP;

  ctrlr->rtrigger = 0;
  ctrlr->ltrigger = 0; // 255;
  ctrlr->joyx = 0;
  ctrlr->joyy = 0;
  ctrlr->UP = BUTTON_UP;
  ctrlr->DOWN = BUTTON_UP;
  ctrlr->LEFT = BUTTON_UP;
  ctrlr->RIGHT = BUTTON_UP;
  ctrlr->START = BUTTON_UP;

  player_t *p = (player_t *)aictrl->source;

  if (aictrl->goal.expiration_frame == g_point->round_time) {
    ai_set_next_goal(abc);
    read_aibot_controller(abc, ctrlr);
    return;
  }
  switch (aictrl->goal.action) {
  case AI_GOAL_MOVE_TO:
    if (_move_towards(abc, ctrlr, &aictrl->goal.description.move_to.target,
                      aictrl->goal.description.move_to.speed,
                      aictrl->goal.description.move_to.airbrake, NULL)) {
      ai_set_next_goal(abc);
      read_aibot_controller(abc, ctrlr);
      break;
    }
    break;

  case AI_GOAL_GETJUIZED:
    if (p->juize >= aictrl->goal.description.charge.juize_to) {
      ai_set_next_goal(abc);
      read_aibot_controller(abc, ctrlr);
      break;
    }
    ctrlr->BTN_X = BUTTON_DOWN;
    ctrlr->rtrigger = 255;
    ctrlr->ltrigger = 255;
    break;
  case AI_GOAL_JUIZE2SHIELD:
    ctrlr->rtrigger = 255;
    if (p->health / (float)PLAYER_START_HEALTH >=
        aictrl->goal.description.juize2shield.health_pct) {
      ai_set_next_goal(abc);
      read_aibot_controller(abc, ctrlr);
      break;
    }
    ctrlr->BTN_Y = BUTTON_DOWN;
    ctrlr->rtrigger = 255;
    if (p->juize < COST_HEALTH) {
      ctrlr->ltrigger = 255;
    }
    break;

  case AI_GOAL_MINES:
    ctrlr->rtrigger = 255;
    if (aictrl->goal.description.mine.num_mines <= 0) {
      ai_set_next_goal(abc);
      read_aibot_controller(abc, ctrlr);
      break;
    }
    if (p->juize < COST_MINE) {
      ctrlr->ltrigger = 255;
      ctrlr->BTN_X = BUTTON_DOWN;
    }
    if (aictrl->goal.description.mine.cooldown) {
      aictrl->goal.description.mine.cooldown--;
      break;
    }
    if (p->button.b == 0) {
      ctrlr->BTN_B = BUTTON_DOWN_THIS_FRAME;
    } else {
      ctrlr->BTN_B = BUTTON_UP;
    }
    if (p->dragged_mine != NULL) {
      aictrl->goal.description.mine.num_mines--;
      aictrl->goal.description.mine.cooldown = 8 + rand() % 6;
    }
    break;
  case AI_GOAL_SHOOT:
    float rot_diff;
    if (p->juize < COST_SHOT) {
      ctrlr->BTN_X = BUTTON_DOWN;
    }
    _move_towards(
        abc, ctrlr,
        &((player_t *)aictrl->goal.description.shoot.target)->position, 1.0f, 1,
        &rot_diff);

    if (rot_diff > 0.85f) {
      ctrlr->BTN_A = BUTTON_DOWN;
      aictrl->goal.description.shoot.num_shots--;
      aictrl->goal.expiration_frame += 25;
    }
    if (aictrl->goal.description.shoot.num_shots <= 0) {
      ai_set_next_goal(abc);
      read_aibot_controller(abc, ctrlr);
      return;
    }
    break;

  case AI_GOAL_GLIDEMINE:
    _move_towards(
        abc, ctrlr,
        &((player_t *)aictrl->goal.description.glidebomb.target)->position,
        1.0f, 0, NULL);
    ctrlr->rtrigger = 255;
    if ((p->speed < 1.0f / 3.0f) || (p->size > (MAX_PLAYER_SIZE * 0.5f))) {
      break;
    }
    if (p->dragged_mine == NULL && p->juize < COST_MINE) {
      ctrlr->BTN_X = BUTTON_DOWN;
      break;
    }
    ctrlr->BTN_B = BUTTON_DOWN;
    if (p->dragged_mine != NULL && p->button.b > MINE_PULSE_RATE) {
      ctrlr->BTN_B = BUTTON_UP_THIS_FRAME;
      aictrl->goal.description.glidebomb.num_bombs = 0;

      float esc_dir = atan2f(p->velocity.y, p->velocity.x) +
                      (rand() % 1 > 0 ? 0.25f : -0.25f) * F_PI;

      vec2d_t moveto = {p->position.x + fcos(esc_dir) * 4.0f,
                        p->position.y + fsin(esc_dir) * 4.0f};

      _ai_set_move2(aictrl, moveto, 1.0f, 0);
      aictrl->goal.expiration_frame = g_point->round_time + 15;
    }
    break;
  case AI_GOAL_COUNT:
    DEBUG_PRINT("called with AI_GOAL_COUNT!\n");
  default:
    DEBUG_PRINT("Unknown AI goal %d\n", aictrl->goal.action);
    break;
  }

  // aictrl->avoid_mine_frames--;
  // if (aictrl->avoid_mine_frames < 0) {
  //   aictrl->avoid_mine = NULL;
  //   aictrl->avoid_mine_frames = 0;
  // }
  // if (aictrl->avoid_mine == NULL) {
  //   aictrl->avoid_mine = _nearest_mine(abc, ctrlr);
  //   if (aictrl->avoid_mine != NULL) {
  //     aictrl->avoid_mine_frames = 3;
  //   }
  // }
  _avoid_nearest_mine(abc, ctrlr);
  // ctrlr->rtrigger = 0;
}
