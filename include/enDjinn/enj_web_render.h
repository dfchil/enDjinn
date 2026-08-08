#ifndef ENJ_WEB_RENDER_H
#define ENJ_WEB_RENDER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int viewport_x;
  int viewport_y;
  int viewport_width;
  int viewport_height;
  int render_width;
  int render_height;
} enj_web_render_pass_context_t;

typedef void (*enj_web_render_pass_callback_t)(
    const enj_web_render_pass_context_t *context, const void *data);

typedef enum {
  ENJ_WEB_RENDER_PASS_BACKGROUND = 0,
  ENJ_WEB_RENDER_PASS_FOREGROUND = 1,
} enj_web_render_pass_phase_t;

typedef struct {
  uint32_t textured;
  void *texture;
  uint32_t texture_format;
  uint32_t texture_width;
  uint32_t texture_height;
  uint32_t texture_filter;
} enj_web_texture_t;

/*
 * Queue an application-owned WebGL pass for the current scene. Passes run
 * after the default framebuffer is cleared and before queued PVR primitives.
 * The data payload is copied when the pass is submitted.
 */
void enj_web_render_pass_submit(enj_web_render_pass_callback_t callback,
                                const void *data, uint32_t data_size);

/*
 * Queue a pass at an explicit scene phase. Foreground passes run after all
 * queued PVR primitives and are intended for depth-tested translucent effects.
 */
void enj_web_render_pass_submit_at(enj_web_render_pass_phase_t phase,
                                   enj_web_render_pass_callback_t callback,
                                   const void *data, uint32_t data_size);

/*
 * Resolve and bind a PVR texture through enDjinn's WebGL texture cache.
 * The active texture unit is left unchanged.
 */
void enj_web_texture_bind(const enj_web_texture_t *texture);

#ifdef __cplusplus
}
#endif

#endif  // ENJ_WEB_RENDER_H
