#ifndef GAME_MODE_H
#define GAME_MODE_H

typedef void (*gameupdate_fun)(void *data);

typedef struct {
	char name[32];
	gameupdate_fun mode_updater;
	void *data;
} game_mode_t;

void mode_set(game_mode_t* mode);

game_mode_t* mode_get(void);

int mode_push(game_mode_t *mode);
void mode_pop(void);

void mode_goto_index(int index);

unsigned int mode_get_index(void);

#include "core.inl.h"

#endif // GAME_MODE_H
