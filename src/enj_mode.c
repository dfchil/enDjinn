#include <enDjinn/enj_enDjinn.h>
#include <stddef.h>

static int current_mode_index = -1;
alignas(32) static enj_mode_t* mode_stack[ENJ_MODE_STACK_SIZE] = {NULL};

void enj_mode_set(enj_mode_t* mode) {
    mode_stack[current_mode_index] = mode;
}

enj_mode_t* enj_mode_get(void) { return mode_stack[current_mode_index]; }

int enj_mode_push(enj_mode_t* mode) {
#ifdef ENJ_DEBUG
    if (current_mode_index >= ENJ_MODE_STACK_SIZE) {
        ENJ_DEBUG_PRINT("Mode stack overflow\n");
        return -1;
    }
#endif
    if (current_mode_index < 0) {
        current_mode_index = 0;
        enj_mode_set_title_screen_ref(0, NULL);
    } else {
        current_mode_index++;
    }
    mode_stack[current_mode_index] = mode;
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

void enj_mode_goto_index(int index, enj_mode_set_fun prepare_fun) {
#ifdef ENJ_DEBUG
    if (index > current_mode_index) {
        ENJ_DEBUG_PRINT("Mode stack index out of bounds\n");
        return;
    }
#endif
    for (int i = current_mode_index; i > index; i--) {
        enj_mode_pop();
        if (prepare_fun) {
            prepare_fun(enj_mode_get());
        }
    }
}

int enj_mode_get_index(void) { return (unsigned int)current_mode_index; }

void enj_mode_set_title_screen_ref(int index, enj_mode_set_fun trans_fun) {
    enj_state_get()->title_screen_mode_stack_index = index;
    enj_state_get()->title_screen_jump_fun = trans_fun;
}

void enj_mode_set_transition_getter(enj_mode_transition_getter trans_fun) {
    enj_state_get()->mode_transition_getter = trans_fun;
}
int enj_mode_cut_to_title_screen(void) {
    int index = enj_state_get()->title_screen_mode_stack_index;
    if (index < 0) {
        ENJ_DEBUG_PRINT("Title screen index not set!\n");
        enj_shutdown_flag();
        return 0;
    }
    enj_mode_goto_index(index, enj_state_get()->title_screen_jump_fun);
    return 1;
}

void enj_mode_flag_end(void) { enj_state_get()->flags.end_mode = 1; }