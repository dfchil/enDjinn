#ifndef ENJ_GAME_MODE_H
#define ENJ_GAME_MODE_H

typedef struct enj_mode_s {
	void (*mode_updater)(void* data);
	void * data;
	void (*pop_fun)(struct enj_mode_s *prev_mode, struct enj_mode_s *next_mode);
	char name[20];
} enj_mode_t;

void enj_mode_set(enj_mode_t* mode);

enj_mode_t* enj_mode_get(void);

enj_mode_t* enj_mode_get_by_index(int index);

int enj_mode_push(enj_mode_t *mode);

enj_mode_t* enj_mode_pop();

int enj_mode_get_current_index(void);

void enj_mode_set_soft_reset_target(int index);

int enj_mode_cut_to_soft_reset_target(void);

void enj_mode_goto_index(int index);

void enj_mode_flag_end_current(void);


#endif // ENJ_GAME_MODE_H
