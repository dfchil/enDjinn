#ifndef ENJ_GAME_MODE_H
#define ENJ_GAME_MODE_H

typedef void (*enj_mode_update_fun)(void *data);

typedef struct enj_game_mode_s {
	char name[32];
	enj_mode_update_fun mode_updater;
	void *data;
} enj_game_mode_t;

typedef void (*enj_mode_set_fun)(enj_game_mode_t *mode);
typedef enj_game_mode_t *(*enj_mode_transition_getter)(void);

void enj_mode_set(enj_game_mode_t* mode);

enj_game_mode_t* enj_mode_get(void);

int enj_mode_push(enj_game_mode_t *mode);

void enj_mode_pop(void);

unsigned int enj_mode_get_index(void);

void enj_mode_set_title_screen_ref(int index, enj_mode_set_fun trans_fun);

void enj_mode_set_transition_getter(enj_mode_transition_getter trans_fun);

int enj_mode_cut_to_title_screen(void);

void enj_mode_goto_index(int index, enj_mode_set_fun prepare_fun);

void enj_mode_flag_end(void);


#endif // ENJ_GAME_MODE_H
