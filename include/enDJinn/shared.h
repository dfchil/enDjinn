#ifndef SHARED_H
#define SHARED_H

// #include <dc/maple.h>
// #include <dc/maple/controller.h>
// #include <dc/matrix.h>
// #include <dc/matrix3d.h>
// #include <dc/perfctr.h>
// #include <dc/pvr.h>
// #include <kos/init.h>
// #include <math.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/param.h>

// #define OLD_PI 3.141592f
// #define PI 3.141592f
// #define PI2 6.283185f
// #define PI_2 1.570796f
// #define PI_16 0.19634954f
// #define PI_32 0.09817477042f


// /*shot_t types*/
// #define SHOT 1
// #define EXPLOSION 2
// #define MINE_EXPLOSION 5

// /*game rules*/
// #define TEAMPLAY 0
// #define ENGINE_SOUNDS 1
// #define AIR_DENSITY 0.05
// #define JOYSTICK_SENSITIVITY 0.0004
// #define PLAYER_ACCELERATION 0.00003
// #define SHOT_SPEED 0.7f
// #define MINE_FRAGMENT_SPEED (SHOT_SPEED * 1.5f)

// #define PLAYER_START_HEALTH 270
// #define ASSAILANT_EARNS_HEALTH 0.33
// #define PARTICLE_SIZE 0.20f
// #define TRAIL_LENGTH 64

// #define MAX_NUM_PLAYERS 8

// #define SHOT_REPEAT_DELAY 15
// #define MIN_PLAYER_SIZE 0.850f
// #define MAX_PLAYER_SIZE 3.0f

// #define MINE_PULSE_RATE 30
// #define MINE_SIZE 0.6f
// #define MINE_FRAGMENT_SIZE 0.08f;
// #define MINE_COUNTDOWN 590

// #define MINE_FRAGMENT_STRENGTH 36
// #define MINE_DECREASE_PER_TURN 5
// #define MINE_NUM_FRAGMENTS 32

// #define MINE_EXPLOSION_PUSH 0.03f

// #define COST_SHOT 10.0f
// #define COST_MINE 15.0f
// #define COST_HEALTH 2.5f
// #define COST_JUIZE 1.5f

// #define JUIZE_CHARGE_RATE 0.13f

// #define MAX_SHOT_STRENGTH 160
// #define SHOT_HEALTH_MODIFIER 0.333f

// #define SHOT_PLAYER_PUSH 0.001
// #define SHOT_MINE_PUSH 0.002f

// typedef enum {
//   PLAYER_E = 1,
//   SHOT_E,
//   MINE_E,
//   // TRAIL_E,
//   IMPACT_E,
//   // MINE_EXP_E,
//   SHIPWRECK_E,
//   PARTICLE_E,
//   ENTITIES_COUNT_E
// } entity_t;

// typedef struct __attribute__((aligned(4))) {
//   float x;
//   float y;
// } vec2d_t;

// typedef struct __attribute__((aligned(4))) {
//   float x;
//   float y;
//   float z;
// } vec3d_t;

// typedef struct __attribute__((aligned(4))) {
//   vec2d_t position;
//   vec2d_t velocity;
//   float radius;
// } circle_t;

// #define print_bits(x)                                                          \
//   do {                                                                         \
//     typeof(x) a__ = (x);                                                       \
//     char *p__ = (char *)&a__ + sizeof(x) - 1;                                  \
//     size_t bytes__ = sizeof(x);                                                \
//     DEBUG_PRINT(#x ": ");                                                      \
//     while (bytes__--) {                                                        \
//       char bits__ = 8;                                                         \
//       while (bits__--)                                                         \
//         putchar(*p__ & (1 << bits__) ? '1' : '0');                             \
//       p__--;                                                                   \
//     }                                                                          \
//     putchar('\n');                                                             \
//   } while (0)

// void game_mem_init(void);

#endif // SHARED_H
