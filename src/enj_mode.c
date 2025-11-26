#include <enDjinn/enj_enDjinn.h>
#include <stddef.h>

static int current_mode_index = -1;
alignas(32) static enj_mode_t* mode_stack[ENJ_MODE_STACK_SIZE] = {NULL};

void enj_mode_set(enj_mode_t* mode) { mode_stack[current_mode_index] = mode; }

enj_mode_t* enj_mode_get(void) { return mode_stack[current_mode_index]; }

enj_mode_t* enj_mode_get_by_index(int index) {
    if (index < 0 || index > current_mode_index) {
        ENJ_DEBUG_PRINT("Mode stack index out of bounds\n");
        return NULL;
    }
    return mode_stack[index];
}

int enj_mode_push(enj_mode_t* mode) {
#ifdef ENJ_DEBUG
    if (current_mode_index >= ENJ_MODE_STACK_SIZE) {
        ENJ_DEBUG_PRINT("Mode stack overflow\n");
        return -1;
    }
#endif
    if (current_mode_index < 0) {
        current_mode_index = 0;
    } else {
        if (mode->trans_fun != NULL) {
            mode->trans_fun(mode_stack[current_mode_index], mode);
            current_mode_index++;
        }
        mode_stack[current_mode_index] = mode;
        return 0;
    }
}
enj_mode_t* enj_mode_pop(void) {
#ifdef ENJ_DEBUG
    if (current_mode_index <= 0) {
        ENJ_DEBUG_PRINT("Mode stack underflow\n");
        return;
    }
#endif
    enj_mode_t* popped_mode = mode_stack[current_mode_index];
    mode_stack[current_mode_index] = NULL;
    current_mode_index--;

    if (mode_stack[current_mode_index]->trans_fun != NULL &&
        popped_mode != NULL) {
        mode_stack[current_mode_index]->trans_fun(
            popped_mode, mode_stack[current_mode_index]);
    }
    return popped_mode;
}

void enj_mode_goto_index(int index) {
#ifdef ENJ_DEBUG
    if (index > current_mode_index) {
        ENJ_DEBUG_PRINT("Mode stack index out of bounds\n");
        return;
    }
#endif
    enj_mode_t* prev = mode_stack[current_mode_index];
    for (int i = current_mode_index; i > index; i--) {
        mode_stack[i] = NULL;
    }
    enj_mode_t* next = enj_mode_get();
    if (prev != NULL && next != NULL && next->trans_fun != NULL) {
        next->trans_fun(prev, next);
    }
}

int enj_mode_get_current_index(void) {
    return (unsigned int)current_mode_index;
}

void enj_mode_set_title_screen_index(int index) {
    enj_state_get()->title_screen_mode_stack_index = index;
}

int enj_mode_cut_to_title_screen(void) {
    int index = enj_state_get()->title_screen_mode_stack_index;
    if (index < 0) {
        ENJ_DEBUG_PRINT("Title screen index not set!\n");
        enj_shutdown_flag();
        return 0;
    }
    enj_mode_goto_index(index);
    return 1;
}

void enj_mode_flag_end_current(void) { enj_state_get()->flags.end_mode = 1; }