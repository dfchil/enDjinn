#include <dc/video.h>
#ifdef ENJ_DBG_PRINT
#include <dc/perf_monitor.h>
#endif
#include <enDjinn/enj_enDjinn.h>
#include <malloc.h>

#define RENDERLIST_SEGMENT_SIZE 64
#define NUM_RENDERLISTS PVR_LIST_PT_POLY - PVR_LIST_OP_POLY + 1

typedef struct enj_render_item_s {
  void (*renderer)(void *data);
  void *data;
} enj_render_item_t;

typedef struct enj_renderlist_s {
  int count;
  struct enj_renderlist_s *next_list;
  alignas(32) enj_render_item_t items[RENDERLIST_SEGMENT_SIZE];
} enj_renderlist_t;

alignas(32) static uint8_t num_allocations[NUM_RENDERLISTS] = {0};
alignas(32) static enj_renderlist_t *first_renderlists[NUM_RENDERLISTS] = {0};
alignas(32) static enj_renderlist_t *active_renderlists[NUM_RENDERLISTS] = {0};

/** optionally set a callback method after each rendering iteration */
static void *_render_post_data = NULL;
static void (*_render_post_call)(void *) = NULL;

static pvr_palfmt_t enj_palette_mode_switch;
static int enj_palette_mode_switch_pending;

void enj_render_palette_mode_set(pvr_palfmt_t mode) {
  enj_palette_mode_switch = mode;
  enj_palette_mode_switch_pending = 1;
}

void enj_render_post_callback_set(void (*post_call)(void *), void *data) {
  _render_post_call = post_call;
  _render_post_data = data;
}

void enj_render_list_add(pvr_list_t renderlist, void (*renderer)(void *data),
                         void *data) {
  if (renderlist > PVR_LIST_PT_POLY || renderlist < PVR_LIST_OP_POLY) {
    ENJ_DEBUG_PRINT("Error: renderlist out of bounds\n");
    return;
  }
  if (renderer == NULL) {
    ENJ_DEBUG_PRINT("Error: renderer callback is NULL\n");
    return;
  }
  // if (renderlist == PVR_LIST_OP_POLY) {
  //   renderer(data);
  //   return;
  // }
  enj_renderlist_t *list = active_renderlists[renderlist];

  if (list == NULL) {
    list = memalign(32, sizeof(enj_renderlist_t));
    if (!list) {
      ENJ_DEBUG_PRINT("Error: renderlist allocation failed\n");
      return;
    }
    list->next_list = NULL;
    list->count = 0;
    list->next_list = NULL;
    active_renderlists[renderlist] = first_renderlists[renderlist] = list;
    num_allocations[renderlist]++;
  }
  if (list->count == RENDERLIST_SEGMENT_SIZE) {
    if (list->next_list == NULL) {
      list->next_list = (void *)memalign(32, sizeof(enj_renderlist_t));
      if (!list->next_list) {
        ENJ_DEBUG_PRINT("Error: renderlist next allocation failed\n");
        return;
      }
      list = list->next_list;
      list->next_list = NULL;
      num_allocations[renderlist]++;
    } else {
      list = list->next_list;
    }
    list->count = 0;
    active_renderlists[renderlist] = list;
  }
  list->items[list->count].renderer = renderer;
  list->items[list->count].data = data;
  list->count++;
}

void enj_render_next_frame(enj_mode_t *current_updater) {
  if (current_updater == NULL || current_updater->mode_updater == NULL) {
    ENJ_DEBUG_PRINT("Error: current mode has no updater\n");
    return;
  }
  // clean up renderlists
  for (int i = PVR_LIST_OP_POLY; i <= PVR_LIST_PT_POLY; i++) {
    active_renderlists[i] = first_renderlists[i];
    enj_renderlist_t *list = active_renderlists[i];
    while (list != NULL) {
      list->count = 0;
      list = list->next_list;
    }
  }

#if ENJ_SHOWFRAMETIMES == 1
  vid_border_color(0, 0, 255);
  uint64_t phase_start_ns = timer_ns_gettime64();
#endif

#ifdef ENJ_DBG_PRINT
  perf_monitor();
#endif

  // update game logic and build custom rendering list
  current_updater->mode_updater(current_updater->data);

#if ENJ_SHOWFRAMETIMES == 1
  uint64_t phase_end_ns = timer_ns_gettime64();
  uint64_t update_ns = phase_end_ns - phase_start_ns;
  vid_border_color(255, 255, 0);
  phase_start_ns = phase_end_ns;
#endif
  pvr_wait_ready();
#if ENJ_SHOWFRAMETIMES == 1
  phase_end_ns = timer_ns_gettime64();
  uint64_t wait_ns = phase_end_ns - phase_start_ns;
  uint64_t render_ns = 0;
  vid_border_color(0, 255, 0);
  phase_start_ns = phase_end_ns;
#endif
  pvr_scene_begin();
  for (int rlist = PVR_LIST_OP_POLY; rlist <= PVR_LIST_PT_POLY; rlist++) {
    pvr_list_begin(rlist);
    if (first_renderlists[rlist] != NULL) {
      enj_renderlist_t *list = first_renderlists[rlist];
      while (list != NULL) {
        for (int j = 0; j < list->count; j++) {
          list->items[j].renderer(list->items[j].data);
        }
        list->count = 0;
        list = list->next_list;
      }
    }
    pvr_list_finish();
  }
  if (enj_palette_mode_switch_pending) {
#if ENJ_SHOWFRAMETIMES == 1
    phase_end_ns = timer_ns_gettime64();
    render_ns += phase_end_ns - phase_start_ns;
    vid_border_color(255, 255, 0);
    phase_start_ns = phase_end_ns;
#endif
    pvr_wait_render_done();
#if ENJ_SHOWFRAMETIMES == 1
    phase_end_ns = timer_ns_gettime64();
    wait_ns += phase_end_ns - phase_start_ns;
    vid_border_color(0, 255, 0);
    phase_start_ns = phase_end_ns;
#endif
    pvr_set_pal_format(enj_palette_mode_switch);
    enj_palette_mode_switch_pending = 0;
  }
#if ENJ_SHOWFRAMETIMES == 1
  phase_end_ns = timer_ns_gettime64();
  render_ns += phase_end_ns - phase_start_ns;
  vid_border_color(255, 255, 0);
  phase_start_ns = phase_end_ns;
#endif
  pvr_scene_finish();
#if ENJ_SHOWFRAMETIMES == 1
  phase_end_ns = timer_ns_gettime64();
  wait_ns += phase_end_ns - phase_start_ns;
  vid_border_color(0, 255, 0);
  phase_start_ns = phase_end_ns;
#endif

  if (_render_post_call != NULL) {
    _render_post_call(_render_post_data);
  }
#if ENJ_SHOWFRAMETIMES == 1
  phase_end_ns = timer_ns_gettime64();
  render_ns += phase_end_ns - phase_start_ns;
  vid_border_color(255, 0, 0);
#endif
}

void enj_render_print_list_sizes(void) {
#ifdef ENJ_DBG_PRINT
  size_t total_bytes = 0;
  for (int i = PVR_LIST_OP_POLY; i <= PVR_LIST_PT_POLY; i++) {
    total_bytes += num_allocations[i] * sizeof(enj_renderlist_t);
    ENJ_DEBUG_PRINT("Renderlist %d, entries: %zu, bytes: %zu\n", i,
                    num_allocations[i] * RENDERLIST_SEGMENT_SIZE,
                    num_allocations[i] * sizeof(enj_renderlist_t));
  }
  ENJ_DEBUG_PRINT("Total bytes used in renderlists: %zu\n\n", total_bytes);
  ENJ_DEBUG_PRINT(
      "***************************************************************\n");
#endif
}
