#ifndef ENDJINN_RENDER_H
#define ENDJINN_RENDER_H

#include <enDjinn/enj_enDjinn.h>

typedef void (*enj_render_method)(void *data);

typedef void (*enj_render_post_call)(void *data);

void enj_render_post_callback(enj_render_post_call cleanup, void *data);

void enj_render_set_palette_mode(int mode);

void enj_next_frame(enj_game_mode_t* updater);

void enj_print_renderlist_sizes(void);

void enj_renderlist_add(pvr_list_t renderlist, enj_render_method renderer,
                       void *data);

#endif // ENDJINN_RENDER_H