#ifndef PC_ENDJINN_KOS_H
#define PC_ENDJINN_KOS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include <stdlib.h>

#ifndef __unused
#define __unused __attribute__((unused))
#endif

static inline void *pc_kos_memalign(size_t alignment, size_t size) {
  void *ptr = NULL;
  return posix_memalign(&ptr, alignment, size) == 0 ? ptr : NULL;
}

#ifndef memalign
#define memalign(alignment, size) pc_kos_memalign((alignment), (size))
#endif

typedef uint32_t uint32;
typedef uint8_t uint8;
typedef void *pvr_ptr_t;

typedef enum pvr_list {
  PVR_LIST_OP_POLY = 0,
  PVR_LIST_TR_POLY = 2,
  PVR_LIST_PT_POLY = 4,
} pvr_list_t;
typedef pvr_list_t pvr_list_type_t;

typedef enum pvr_palfmt {
  PVR_PAL_ARGB1555 = 0,
  PVR_PAL_RGB565 = 1,
  PVR_PAL_ARGB4444 = 2,
  PVR_PAL_ARGB8888 = 3,
} pvr_palfmt_t;

typedef enum vid_pixel_mode { PM_RGB565 = 0, PM_RGB888P = 1 } vid_pixel_mode_t;
typedef vid_pixel_mode_t pvr_pixel_mode_t;
typedef enum vid_display_mode_generic { DM_640x480 = 0 } vid_display_mode_generic_t;

typedef struct vid_mode {
  int width;
  int height;
} vid_mode_t;

typedef struct pvr_init_params {
  int opb_sizes[5];
  int vertex_buf_size;
  int dma_enabled;
  int fsaa_enabled;
  int autosort_disabled;
  int opb_overflow_count;
  int vbuf_doublebuf_disabled;
} pvr_init_params_t;

typedef struct pvr_context_gen { int culling; int fog_type; } pvr_context_gen_t;
typedef struct pvr_sprite_cxt { pvr_context_gen_t gen; } pvr_sprite_cxt_t;
typedef struct pvr_poly_cxt { pvr_context_gen_t gen; } pvr_poly_cxt_t;
typedef struct {
  alignas(32) uint32_t cmd;
  uint32_t mode1;
  uint32_t mode2;
  uint32_t mode3;
  uint32_t argb;
  uint32_t oargb;
  uint32_t reserved[2];
} pvr_sprite_hdr_t;
typedef pvr_sprite_hdr_t pvr_poly_hdr_t;

typedef struct pvr_vertex {
  alignas(32) uint32_t flags;
  float x, y, z, u, v;
  uint32_t argb, oargb;
} pvr_vertex_t;

typedef struct pvr_sprite_col {
  alignas(32) uint32_t flags;
  float ax, ay, az, bx, by, bz, cx, cy, cz, dx, dy;
  uint32_t d1, d2, d3, d4;
} pvr_sprite_col_t;

typedef struct pvr_sprite_txr {
  alignas(32) uint32_t flags;
  float ax, ay, az, bx, by, bz, cx, cy, cz, dx, dy;
  uint32_t dummy;
  uint32_t auv, buv, cuv;
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

#define MAPLE_PORT_COUNT 4
#define MAPLE_UNIT_COUNT 6
#define MAPLE_FUNC_CONTROLLER 0x01000000u
#define MAPLE_FUNC_LCD 0x04000000u
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
  struct { uint32_t functions; } info;
} maple_device_t;
typedef void (*maple_user_callback_t)(maple_device_t *device, void *user_data);

typedef struct cont_state {
  union {
    uint32_t buttons;
    struct {
      uint32_t c : 1, b : 1, a : 1, start : 1;
      uint32_t dpad_up : 1, dpad_down : 1, dpad_left : 1, dpad_right : 1;
      uint32_t z : 1, y : 1, x : 1, d : 1;
      uint32_t dpad2_up : 1, dpad2_down : 1, dpad2_left : 1,
          dpad2_right : 1;
      uint32_t reserved : 16;
    };
  };
  int ltrig, rtrig;
  int joyx, joyy, joy2x, joy2y;
} cont_state_t;

typedef union purupuru_effect {
  uint32_t raw;
  struct {
    uint32_t cont : 1, res : 3, motor : 4;
    uint32_t bpow : 3, div : 1, fpow : 3, conv : 1;
    uint32_t freq : 8, inc : 8;
  };
} purupuru_effect_t;
typedef int sfxhnd_t;
#define SFXHND_INVALID (-1)

typedef struct fDtHeader {
  uint32_t format;
  uint16_t width;
  uint16_t height;
} fDtHeader;

#ifdef __cplusplus
extern "C" {
#endif

extern vid_mode_t *vid_mode;
uint64_t timer_ns_gettime64(void);
void vid_border_color(uint8_t r, uint8_t g, uint8_t b);
void vid_set_mode(vid_display_mode_generic_t mode, vid_pixel_mode_t pixel_mode);
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

maple_device_t *maple_enum_type(int index, uint32_t function);
maple_device_t *maple_enum_dev(int port, int unit);
void *maple_dev_status(maple_device_t *device);
void maple_attach_callback(
    uint32_t function, maple_user_callback_t callback, void *user_data);
void maple_detach_callback(
    uint32_t function, maple_user_callback_t callback, void *user_data);

void snd_init(void);
sfxhnd_t snd_sfx_load_raw_buf(
    void *samples, size_t size, uint32_t sample_rate, uint8_t bits, uint8_t channels);
void snd_sfx_unload(sfxhnd_t handle);
int snd_sfx_play(sfxhnd_t handle, uint8_t volume, uint8_t pan);
void purupuru_rumble_raw(maple_device_t *device, uint32_t effect);
void perf_monitor(void);

#ifdef __cplusplus
}
#endif

static inline void pvr_sprite_cxt_col(pvr_sprite_cxt_t *cxt, pvr_list_t list) {
  (void)list;
  cxt->gen.culling = PVR_CULLING_CCW;
  cxt->gen.fog_type = PVR_FOG_TABLE;
}
static inline void pvr_poly_cxt_col(pvr_poly_cxt_t *cxt, pvr_list_t list) {
  (void)list;
  cxt->gen.culling = PVR_CULLING_CCW;
  cxt->gen.fog_type = PVR_FOG_TABLE;
}
static inline void pvr_sprite_compile(pvr_sprite_hdr_t *hdr, const pvr_sprite_cxt_t *cxt) {
  (void)cxt;
  hdr->cmd = 0u;
  hdr->argb = 0xffffffffu;
}
static inline void pvr_poly_compile(pvr_poly_hdr_t *hdr, const pvr_poly_cxt_t *cxt) {
  (void)cxt;
  hdr->cmd = 0u;
  hdr->argb = 0xffffffffu;
}

#endif
