#ifndef ENJ_GAME_MODE_H
#define ENJ_GAME_MODE_H

typedef struct enj_mode_s {
	void (*mode_updater)(void* data);
	void * data;
	void (*pop_fun)(struct enj_mode_s *prev_mode, struct enj_mode_s *next_mode);
	char name[20];
} enj_mode_t;

/**
 * Push a new mode onto the mode stack and make it the current mode
 * @param mode Pointer to the mode to push onto the stack
 * @return 1 on success, 0 on failure (stack overflow)
 */
int enj_mode_push(enj_mode_t *mode);

/**
 * Replace the current active mode with the given mode
 * @param mode Pointer to the mode to set as current
 */
void enj_mode_set(enj_mode_t* mode);

/**
 * Get the current active mode
 * @return Pointer to the current mode
 */
enj_mode_t* enj_mode_get(void);

/**
 * Get a mode by its index in the mode stack
 * @param index Index of the mode to retrieve
 * @return Pointer to the mode at the given index, or NULL if out of bounds
 */
enj_mode_t* enj_mode_get_by_index(int index);

/**
 * Pop the current mode off the mode stack
 * @return Pointer to the popped mode, or NULL if stack underflow
 */
enj_mode_t* enj_mode_pop();

/**
 * Get the current index of the mode stack
 * @return Current mode stack index
 */
int enj_mode_get_current_index(void);

/**
 * Set the target index for soft resets
 * @param index Index to set as the soft reset target
 */
void enj_mode_soft_reset_target_set(int index);

/**
 * Cut to the soft reset target
 * @return 1 on success, 0 on failure
 */
int enj_mode_cut_to_soft_reset_target(void);

/**
 * Goto a specific index in the mode stack, popping modes as necessary
 * @param index Target index to go to
 */
void enj_mode_goto_index(int index);

/**
 * Flag the current mode to end at the next opportunity
 */
void enj_mode_flag_end_current(void);


#endif // ENJ_GAME_MODE_H
