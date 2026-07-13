#ifndef PC_ENDJINN_DC_PVR_H
#define PC_ENDJINN_DC_PVR_H

#include <stdalign.h>

#include <dc/video.h>
#include <pc_endjinn/types.h>

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
typedef vid_pixel_mode_t pvr_pixel_mode_t;

typedef struct pvr_init_params {
  int opb_sizes[5];
  int vertex_buf_size;
  int dma_enabled;
  int fsaa_enabled;
  int autosort_disabled;
  int opb_overflow_count;
  int vbuf_doublebuf_disabled;
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

PC_ENDJINN_BEGIN_DECLS
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
PC_ENDJINN_END_DECLS

static inline void pvr_sprite_cxt_col(pvr_sprite_cxt_t *cxt,
                                      pvr_list_t list) {
  (void)list;
  cxt->gen.culling = PVR_CULLING_CCW;
  cxt->gen.fog_type = PVR_FOG_TABLE;
}

static inline void pvr_poly_cxt_col(pvr_poly_cxt_t *cxt, pvr_list_t list) {
  (void)list;
  cxt->gen.culling = PVR_CULLING_CCW;
  cxt->gen.fog_type = PVR_FOG_TABLE;
}

static inline void pvr_sprite_compile(pvr_sprite_hdr_t *hdr,
                                      const pvr_sprite_cxt_t *cxt) {
  (void)cxt;
  hdr->cmd = 0u;
  hdr->argb = 0xffffffffu;
}

static inline void pvr_poly_compile(pvr_poly_hdr_t *hdr,
                                    const pvr_poly_cxt_t *cxt) {
  (void)cxt;
  hdr->cmd = 0u;
  hdr->argb = 0xffffffffu;
}

#endif
