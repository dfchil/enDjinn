#include <enDJinn/mode/core.h>
#include <stddef.h>

#define MODE_STACK_SIZE 16

// static game_mode_t *current_mode;
static int current_mode_index = -1;
static game_mode_t *mode_stack[MODE_STACK_SIZE]
    __attribute__((aligned(32))) = {NULL};

void mode_set(game_mode_t *mode) { mode_stack[current_mode_index] = mode; }

game_mode_t *mode_get(void) { return mode_stack[current_mode_index] ; }

int mode_push(game_mode_t *mode) {  
  #ifdef DEBUG
  if (current_mode_index >= MODE_STACK_SIZE) {
    DEBUG_PRINT("Mode stack overflow\n");
    return -1;
  }
  #endif
  mode_stack[++current_mode_index] = mode;
  return 0;
}
void mode_pop(void) {
  #ifdef DEBUG
  if (current_mode_index <= 0) {
    DEBUG_PRINT("Mode stack underflow\n");
    return;
  }
  #endif
  mode_stack[current_mode_index] = NULL;
  current_mode_index--;
}

void mode_goto_index(int index) {
  #ifdef DEBUG
  if (index > current_mode_index) {
    DEBUG_PRINT("Mode stack index out of bounds\n");
    return;
  }
  #endif
  for (int i = current_mode_index; i > index; i--) {
    mode_pop();
  }
}

unsigned int mode_get_index(void) {
  return (unsigned int)current_mode_index;
}