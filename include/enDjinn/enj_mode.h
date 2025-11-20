#ifndef ENJ_GAME_MODE_H
#define ENJ_GAME_MODE_H

typedef void (*gameupdate_fun)(void *data);

typedef struct enj_game_mode_s {
	char name[32];
	gameupdate_fun mode_updater;
	// gamerender_fun mode_renderer;
	void *data;
} enj_game_mode_t;

void enj_mode_set(enj_game_mode_t* mode);

enj_game_mode_t* enj_mode_get(void);

int enj_mode_push(enj_game_mode_t *mode);

void enj_mode_pop(void);

void enj_mode_goto_index(int index);

unsigned int enj_mode_get_index(void);

#endif // ENJ_GAME_MODE_H
