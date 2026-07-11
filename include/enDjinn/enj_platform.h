#ifndef ENJ_PLATFORM_H
#define ENJ_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#if defined(__DREAMCAST__) && !defined(ENJ_TARGET_PC_ENDJINN)
#ifndef ENJ_PLATFORM_DREAMCAST
#define ENJ_PLATFORM_DREAMCAST 1
#endif
#endif

#if ENJ_PLATFORM_DREAMCAST

#include <arch/timer.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/purupuru.h>
#include <dc/maple/vmu.h>
#include <dc/perf_monitor.h>
#include <dc/pvr.h>
#include <dc/pvr/pvr_pal.h>
#include <dc/sound/sfxmgr.h>
#include <dc/video.h>

#else

#include <stdlib.h>

#ifndef __unused
#define __unused __attribute__((unused))
#endif

static inline void *enj_host_memalign(size_t alignment, size_t size)
{
  void *ptr = NULL;
  if (posix_memalign(&ptr, alignment, size) != 0) {
    return NULL;
  }
  return ptr;
}

#ifndef memalign
#define memalign(alignment, size) enj_host_memalign((alignment), (size))
#endif

typedef enum pvr_list {
  PVR_LIST_OP_POLY = 0,
  PVR_LIST_TR_POLY = 1,
  PVR_LIST_PT_POLY = 2,
} pvr_list_t;

typedef pvr_list_t pvr_list_type_t;

typedef enum pvr_palfmt {
  PVR_PAL_ARGB1555 = 0,
  PVR_PAL_RGB565 = 1,
  PVR_PAL_ARGB4444 = 2,
  PVR_PAL_ARGB8888 = 3,
} pvr_palfmt_t;

typedef enum vid_pixel_mode {
  PM_RGB565 = 0,
  PM_RGB888P = 1,
} vid_pixel_mode_t;

typedef vid_pixel_mode_t pvr_pixel_mode_t;

typedef enum vid_display_mode_generic {
  DM_640x480 = 0,
} vid_display_mode_generic_t;

typedef struct enj_host_vid_mode {
  int width;
  int height;
} enj_host_vid_mode_t;

typedef void *pvr_ptr_t;

typedef struct pvr_init_params {
  uint32_t enable_clear;
  uint32_t dma_enabled;
  uint32_t fsaa_enabled;
  uint32_t opb_overflow_count;
  uint32_t vbuf_doublebuf_disabled;
  uint32_t autosort_disabled;
  uint32_t vertex_buf_size;
  uint32_t opb_sizes[5];
} pvr_init_params_t;

typedef struct pvr_context_gen {
  int culling;
  int fog_type;
} pvr_context_gen_t;

typedef struct pvr_sprite_cxt {
  pvr_context_gen_t gen;
} pvr_sprite_cxt_t;

typedef struct pvr_poly_cxt {
  pvr_context_gen_t gen;
} pvr_poly_cxt_t;

typedef struct pvr_sprite_hdr {
  uint32_t dummy;
  uint32_t argb;
} pvr_sprite_hdr_t;

typedef pvr_sprite_hdr_t pvr_poly_hdr_t;

typedef struct pvr_vertex {
  uint32_t flags;
  float x;
  float y;
  float z;
  float u;
  float v;
  uint32_t argb;
  uint32_t oargb;
} pvr_vertex_t;

typedef struct pvr_sprite_col {
  uint32_t flags;
  float ax;
  float ay;
  float az;
  float bx;
  float by;
  float bz;
  float cx;
  float cy;
  float cz;
  float dx;
  float dy;
} pvr_sprite_col_t;

typedef struct pvr_sprite_txr {
  uint32_t flags;
  float ax;
  float ay;
  float az;
  float bx;
  float by;
  float bz;
  float cx;
  float cy;
  float cz;
  float dx;
  float dy;
  uint32_t auv;
  uint32_t buv;
  uint32_t cuv;
} pvr_sprite_txr_t;

#define PVR_CULLING_CCW 0
#define PVR_FOG_TABLE 0
#define PVR_BINSIZE_0 0
#define PVR_BINSIZE_8 8
#define PVR_BINSIZE_16 16
#define PVR_BINSIZE_32 32
#define PVR_CMD_VERTEX 0xe0000000u
#define PVR_CMD_VERTEX_EOL 0xf0000000u
#define PVR_PACK_16BIT_UV(u, v) (0u)

typedef uint32_t uint32;
typedef uint8_t uint8;

#ifndef MAPLE_PORT_COUNT
#define MAPLE_PORT_COUNT 4
#endif
#ifndef MAPLE_UNIT_COUNT
#define MAPLE_UNIT_COUNT 6
#endif

#define MAPLE_FUNC_CONTROLLER 0x01000000u
#define MAPLE_FUNC_LCD 0x00100000u
#define MAPLE_FUNC_PURUPURU 0x00010000u

#define CONT_C 0x0001u
#define CONT_B 0x0002u
#define CONT_A 0x0004u
#define CONT_START 0x0008u
#define CONT_DPAD_UP 0x0010u
#define CONT_DPAD_DOWN 0x0020u
#define CONT_DPAD_LEFT 0x0040u
#define CONT_DPAD_RIGHT 0x0080u
#define CONT_Z 0x0100u
#define CONT_Y 0x0200u
#define CONT_X 0x0400u
#define CONT_D 0x0800u
#define CONT_DPAD2_UP 0x1000u
#define CONT_DPAD2_DOWN 0x2000u
#define CONT_DPAD2_LEFT 0x4000u
#define CONT_DPAD2_RIGHT 0x8000u

typedef struct maple_device {
  int port;
  int unit;
  bool valid;
  struct {
    uint32_t functions;
  } info;
} maple_device_t;

typedef struct cont_state {
  uint8_t a;
  uint8_t b;
  uint8_t x;
  uint8_t y;
  uint8_t start;
  uint8_t dpad_up;
  uint8_t dpad_down;
  uint8_t dpad_left;
  uint8_t dpad_right;
  uint16_t buttons;
  uint8_t rtrig;
  uint8_t ltrig;
  int8_t joyx;
  int8_t joyy;
  int8_t joy2x;
  int8_t joy2y;
} cont_state_t;

typedef union purupuru_effect {
  uint32_t raw;
} purupuru_effect_t;

typedef int sfxhnd_t;
#ifndef SFXHND_INVALID
#define SFXHND_INVALID (-1)
#endif

typedef struct fDtHeader {
  uint32_t format;
  uint16_t width;
  uint16_t height;
} fDtHeader;

#ifdef __cplusplus
extern "C" {
#endif

extern enj_host_vid_mode_t *vid_mode;

uint64_t timer_ns_gettime64(void);

void vid_border_color(uint8_t r, uint8_t g, uint8_t b);
void vid_set_mode(vid_display_mode_generic_t display_mode, vid_pixel_mode_t pixel_mode);

void pvr_init(const pvr_init_params_t *params);
void pvr_shutdown(void);
void pvr_set_bg_color(float r, float g, float b);
void pvr_wait_ready(void);
void pvr_scene_begin(void);
void pvr_scene_finish(void);
void pvr_list_begin(pvr_list_t list);
void pvr_list_finish(void);
void pvr_wait_render_done(void);
void pvr_set_pal_format(pvr_palfmt_t mode);
void pvr_fog_table_color(float a, float r, float g, float b);
void pvr_fog_table_linear(float start, float end);

void *pvr_dr_target(void);
void pvr_dr_commit(void *ptr);

void pc_endjinn_platform_set_video_size(uint32_t width, uint32_t height);
#if ENJ_TARGET_PC_ENDJINN
void pc_endjinn_platform_set_path_debug(
    uint16_t index,
    uint16_t cursor_index,
    uint16_t count,
    uint32_t offset,
    uint32_t address,
    uint16_t route);
#else
static inline void pc_endjinn_platform_set_path_debug(
    uint16_t index,
    uint16_t cursor_index,
    uint16_t count,
    uint32_t offset,
    uint32_t address,
    uint16_t route)
{
  (void)index;
  (void)cursor_index;
  (void)count;
  (void)offset;
  (void)address;
  (void)route;
}
#endif

#ifdef __cplusplus
}
#endif

static inline void pvr_sprite_cxt_col(pvr_sprite_cxt_t *context, pvr_list_t list)
{
  (void)list;
  if (context != NULL) {
    context->gen.culling = PVR_CULLING_CCW;
    context->gen.fog_type = PVR_FOG_TABLE;
  }
}

static inline void pvr_poly_cxt_col(pvr_poly_cxt_t *context, pvr_list_t list)
{
  (void)list;
  if (context != NULL) {
    context->gen.culling = PVR_CULLING_CCW;
    context->gen.fog_type = PVR_FOG_TABLE;
  }
}

static inline void pvr_sprite_compile(
    pvr_sprite_hdr_t *header,
    const pvr_sprite_cxt_t *context)
{
  (void)context;
  if (header != NULL) {
    header->dummy = 0u;
    header->argb = 0xffffffffu;
  }
}

static inline void pvr_poly_compile(
    pvr_poly_hdr_t *header,
    const pvr_poly_cxt_t *context)
{
  (void)context;
  if (header != NULL) {
    header->dummy = 0u;
    header->argb = 0xffffffffu;
  }
}

#endif

#endif
