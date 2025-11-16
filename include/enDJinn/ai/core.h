#ifndef ENDJINN_AI_CORE_H
#define ENDJINN_AI_CORE_H

// #include <drxlax/shared.h>
// #include <drxlax/types.h>

// // #include <drxlax/game/entity/player.h>
// typedef enum {
//     AI_GOAL_MOVE_TO,
//     AI_GOAL_JUIZE2SHIELD,
//     AI_GOAL_GETJUIZED,
//     AI_GOAL_MINES,
//     AI_GOAL_SHOOT,
//     AI_GOAL_GLIDEMINE,
//     AI_GOAL_COUNT
// } ai_goal_type_e;

// typedef struct {
//     vec2d_t target;
//     float speed;
//     int airbrake;
// } ai_goal_move_to_t;

// typedef struct {
//     int num_bombs;
//     void *target;
// } ai_goal_glidebomb_t;

// typedef struct {
//   struct {
//     int num_mines : 8;
//     int cooldown : 8;
//     int _ : 16;
//   };
// } ai_goal_mines_t;

// typedef struct {
//     int num_shots;
//     void *target;
// } ai_goal_shoot_t;

// typedef struct {
//     int juize_to;
// } AI_GOAL_GETJUIZED_t;

// typedef struct {
//     float health_pct;
// } ai_goal_juize2shield_t;

// typedef struct {
//   ai_goal_type_e action;
//   union {
//     ai_goal_move_to_t move_to;
//     ai_goal_glidebomb_t glidebomb;
//     ai_goal_mines_t mine;
//     ai_goal_shoot_t shoot;
//     AI_GOAL_GETJUIZED_t charge;
//     ai_goal_juize2shield_t juize2shield;
//   } description;
//   uint32_t expiration_frame; 
// } ai_goal_t;


// typedef enum {
//   AI_PROF_EXPLORER_E,
//   AI_PROF_DEFENSIVE_E,
//   AI_PROF_GENERAL_E,
//   AI_PROF_HUNTER_E,
//   AI_PROF_MINER_E,
//   AI_PROF_SHOOTER_E,
//   AI_PROF_GLIDEMINER_E,
//   AI_PROF_COUNT
// } ai_profiles_e;

// const char *ai_get_profile_name(ai_profiles_e profile);
// uint8_t *ai_get_profile(ai_profiles_e profile);


#endif // ENDJINN_AI_CORE_H
