#include <enDjinn/enj_enDjinn.h>
#include <stddef.h>


// static game_mode_t *current_mode;
static int current_mode_index = -1;
static enj_game_mode_t *mode_stack[ENJ_MODE_STACK_SIZE]
    __attribute__((aligned(32))) = {NULL};

void enj_mode_set(enj_game_mode_t *mode) { mode_stack[current_mode_index] = mode; }

enj_game_mode_t *enj_mode_get(void) { return mode_stack[current_mode_index] ; }

int enj_mode_push(enj_game_mode_t *mode) {  
  #ifdef ENJ_DEBUG
  if (current_mode_index >= ENJ_MODE_STACK_SIZE) {
    ENJ_DEBUG_PRINT("Mode stack overflow\n");
    return -1;
  }
  #endif
  mode_stack[++current_mode_index] = mode;
  return 0;
}
void enj_mode_pop(void) {
  #ifdef ENJ_DEBUG
  if (current_mode_index <= 0) {
    ENJ_DEBUG_PRINT("Mode stack underflow\n");
    return;
  }
  #endif
  mode_stack[current_mode_index] = NULL;
  current_mode_index--;
}

void enj_mode_goto_index(int index) {
  #ifdef ENJ_DEBUG
  if (index > current_mode_index) {
    ENJ_DEBUG_PRINT("Mode stack index out of bounds\n");
    return;
  }
  #endif
  for (int i = current_mode_index; i > index; i--) {
    enj_mode_pop();
  }
}

unsigned int enj_mode_get_index(void) {
  return (unsigned int)current_mode_index;
}