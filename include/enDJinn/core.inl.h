#include <arch/arch.h>
#include <dc/pvr/pvr_mem.h>
#include <malloc.h>
#include <time.h>
#include <enDJinn/controller/dreamcast.h>

// #include <drxlax/audio/sfx.h>
// #include <enDJinn/config/config.h>
// #include <enDJinn/controller/rumble.h>
// #include <enDJinn/core.h>
// #include <enDJinn/mode/game.h>
#include <enDJinn/mode/core.h>
// #include <enDJinn/mode/titlescreen.h>
// #include <enDJinn/stats/scoreboard.h>
// #include <enDJinn/transitions/startup_and_shutdown.h>
// #include <enDJinn/transitions/zoom.h>
// #include <drxlax/render/core.h>
// #include <drxlax/render/pvr/perspective.h>
// #include <drxlax/render/pvr/pvr.h>
// #include <drxlax/render/rect_grid.h>
// #include <drxlax/render/textures.h>
// #include <drxlax/shared.h>
// #include <drxlax/types.h>
static uint32_t title_screen_stack_index = 0;

static struct {
  uint32_t end_of_sequence : 1;
  uint32_t shut_down : 1;
  uint32_t : 30;
} flags;

int detect_shutdown_combo(controller_state_t *cstate) {
  if ((cstate->START & BUTTON_DOWN) && (cstate->BTN_A & BUTTON_DOWN)) {
    if ((cstate->START == BUTTON_DOWN_THIS_FRAME) ||
        (cstate->BTN_A == BUTTON_DOWN_THIS_FRAME)) {
      return 1;
    }
  }
  return 0;
}

void core_flag_endofsequence(void) { flags.end_of_sequence = 1; }
void core_flag_shutdown(void) { flags.shut_down = 1; }

static int check_c1_exit(void) {
  return flags.shut_down;
}

void cut_to_title_screen(void) {
  mode_goto_index(title_screen_stack_index);
  // get_title_screen_mode();
  // default_perspective();
  // grid_set_perspective();
  // transform_grid_verts(grid_get());
  // zoomtrans_reset_first_filler();
  // zoomtrans_reset_second_filler();
}

void enDJinn_loop(void) {
  while (1) {
    dc_ctrlrs_map_state();
    if (check_c1_exit()) {

      break;
    }
    if (mode_get_index() != title_screen_stack_index) {
      controller_state_t **cstates = get_ctrlr_states();
      for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
        if (detect_shutdown_combo(cstates[i])) {
          cstates[i]->START = BUTTON_DOWN;
          cstates[i]->BTN_A = BUTTON_DOWN;
          // cut_to_title_screen();
        }
      }
    }

    if (flags.end_of_sequence) {
      mode_pop();
      // mode_push(get_zoom_transition_mode());

      flags.end_of_sequence = 0;
    }
    // render_frame(mode_get());
  }
}

int core_init(void) {

#ifdef DEBUG
  heapUtilization();
#endif
  // g_state_t *g_point = core_get_global();
  // g_point->air_d = AIR_DENSITY; // set air density (eg. resistance)
  // g_point->joy_response = JOYSTICK_SENSITIVITY;  // joy response factor
  // g_point->p_acceleration = PLAYER_ACCELERATION; // acceleration factor
  // g_point->number_of_players = 0;
  // g_point->first_player = 0;
  // g_point->first_offensive = 0;
  // g_point->first_mine = 0;
  // g_point->max_shot_strength = MAX_SHOT_STRENGTH;
  // g_point->shot_speed = SHOT_SPEED;
  // g_point->player_start_health = PLAYER_START_HEALTH;
  // g_point->assailant_earns = ASSAILANT_EARNS_HEALTH;
  // g_point->shot_health_modifier = SHOT_HEALTH_MODIFIER;
  // g_point->particle_size = PARTICLE_SIZE;
  // g_point->mine_strength = MINE_STRENGTH;
  // g_point->sfx_lastchan = 0;
  // g_point->mine_explode_push = 0.03f;
  // g_point->juize_increase = JUIZE_CHARGE_RATE;
  // g_point->mine_countdown = MINE_COUNTDOWN;
  // g_point->cost_shot = COST_SHOT;
  // g_point->cost_mine = COST_MINE;
  // g_point->shot_repeat_delay = SHOT_REPEAT_DELAY;
  // g_point->cost_juize = COST_JUIZE;
  // g_point->cost_health = COST_HEALTH;
  // g_point->shot_player_push = 0.001;
  // g_point->shot_mine_push = 0.002f;

  // g_point->r_control_box.current_frame = 0;

  // rumble_queues_init();
  // game_mem_init();
  // config_reset();
  // srand(time(NULL));
  // if (!init_textures()){
  //   DEBUG_PRINT("Failed to init textures\n");
  //   return 0;
  // }
  // if (!load_plasma()) {
  //   DEBUG_PRINT("Failed to load plasma\n");
  //   return 0;
  // }
  // grid_init(56, 42, 64.0f, 48.0f, 5 * 4.0f / 3.0f, 5 * 3.0f / 4.0f, NULL);

  // scoreboard_reset();
  // game_mode_init();
  // sfx_init();

  // g_point->r_control_box.replay = &replay;
  // replay.data = (c_frame *)memalign(32, 128 << 10);
  // g_point->current_replay = 0;
#ifdef SINGLEDEMO
  // g_point->current_replay = SINGLEDEMO;
#endif
  // mode_push(get_game_ender());
  // mode_push(get_shutdown_mode());
  // mode_push(get_title_screen_mode());

  // menu_state_t *mstate = get_menu_state();
  // mstate->cstates = get_ctrlr_states();
  // mstate->grid = grid_get();
  // mstate->g_point = core_get_global();
  // mstate->active_menu = get_score_menu();
  // mode_push(get_score_menu_mode());

  title_screen_stack_index = mode_get_index();
  // mode_push(get_startup_mode());
#ifdef DEBUG
  heapUtilization();
#endif
  return 1;
}

void heapUtilization(void) {
// #ifdef DEBUG
//   // Query heap manager/allocator for info
//   struct mallinfo mallocInfo = mallinfo();

//   // Used bytes are as reported
//   size_t usedBlocks = mallocInfo.uordblks;
//   // First component of free bytes are as reported
//   size_t freeBlocks = mallocInfo.fordblks;

//   // End address of region reserved for heap growth
//   size_t brkEnd = _arch_mem_top - THD_KERNEL_STACK_SIZE - 1;
//   // Amount of bytes the heap has yet to still grow
//   size_t brkFree = brkEnd - (uintptr_t)sbrk(0);

//   // Total heap space available is free blocks from allocator + unclaimed sbrk()
//   // space
//   freeBlocks += brkFree;

//   DEBUG_PRINT("Heap utilization: %0.3f%%\n",
//               (float)(usedBlocks * 100.0f / (usedBlocks + freeBlocks)));
//   DEBUG_PRINT("freeBlocks: %d\n", freeBlocks);
//   DEBUG_PRINT("usedBlocks: %d\n", usedBlocks);

//   pvr_mem_stats();
//   DEBUG_PRINT("pvr available: %u\n", pvr_mem_available());
//   pvr_mem_print_list();
// #endif
  // print_renderlist_sizes();
}
