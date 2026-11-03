#ifndef PC_ENDJINN_TIMING_H
#define PC_ENDJINN_TIMING_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Where the last submitted scene's time went inside the PC backend, in
 * nanoseconds. A game that times its own frames reads this after
 * pvr_scene_finish() (from its next updater) and lays the backend's phases
 * beside its own. Diagnostic only; nothing in the backend reads it. */
typedef struct enj_pc_frame_timing {
  uint64_t build_ns;    /* host primitives -> vertex batches, translucent sort */
  uint64_t fence_ns;    /* wait for the previous frame's GPU work */
  uint64_t texture_ns;  /* palette and texture decode/upload */
  uint64_t acquire_ns;  /* vkAcquireNextImageKHR (blocks on the swapchain) */
  uint64_t vertex_ns;   /* vertex buffer map/copy, modifier upload */
  uint64_t record_ns;   /* command buffer recording and queue submit */
  uint64_t present_ns;  /* vkQueuePresentKHR (blocks on vsync in FIFO) */
  uint64_t total_ns;    /* the whole pvr_scene_finish() */
  size_t primitives;    /* host primitives queued in the scene */
  size_t vertices;      /* Vulkan vertices built from them */
} enj_pc_frame_timing;

void enj_pc_frame_timing_get(enj_pc_frame_timing *out);

#ifdef __cplusplus
}
#endif

#endif
