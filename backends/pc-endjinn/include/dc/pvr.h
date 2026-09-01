#ifndef PC_ENDJINN_DC_PVR_H
#define PC_ENDJINN_DC_PVR_H

#include <stdalign.h>
#include <stdint.h>
#include <string.h>

#include <dc/video.h>
#include <pc_endjinn/types.h>

typedef void *pvr_ptr_t;

typedef enum pvr_list {
  PVR_LIST_OP_POLY = 0,
  PVR_LIST_OP_MOD = 1,
  PVR_LIST_TR_POLY = 2,
  PVR_LIST_TR_MOD = 3,
  PVR_LIST_PT_POLY = 4,
} pvr_list_t;
typedef pvr_list_t pvr_list_type_t;

typedef enum pvr_palfmt {
  PVR_PAL_ARGB1555 = 0,
  PVR_PAL_RGB565 = 1,
  PVR_PAL_ARGB4444 = 2,
  PVR_PAL_ARGB8888 = 3,
} pvr_palfmt_t;

typedef enum pvr_pixel_mode {
  PVR_PIXEL_MODE_ARGB1555 = 0,
  PVR_PIXEL_MODE_RGB565 = 1,
  PVR_PIXEL_MODE_ARGB4444 = 2,
  PVR_PIXEL_MODE_YUV422 = 3,
  PVR_PIXEL_MODE_BUMP = 4,
  PVR_PIXEL_MODE_PAL_4BPP = 5,
  PVR_PIXEL_MODE_PAL_8BPP = 6,
} pvr_pixel_mode_t;

typedef enum pvr_filter_mode {
  PVR_FILTER_NEAREST = 0,
  PVR_FILTER_BILINEAR = 1,
  PVR_FILTER_TRILINEAR1 = 2,
  PVR_FILTER_TRILINEAR2 = 3,
  PVR_FILTER_NONE = PVR_FILTER_NEAREST,
} pvr_filter_mode_t;

typedef enum pvr_cull_mode {
  PVR_CULLING_NONE,
  PVR_CULLING_SMALL,
  PVR_CULLING_CCW,
  PVR_CULLING_CW,
} pvr_cull_mode_t;

typedef enum pvr_depthcmp_mode {
  PVR_DEPTHCMP_NEVER,
  PVR_DEPTHCMP_LESS,
  PVR_DEPTHCMP_EQUAL,
  PVR_DEPTHCMP_LEQUAL,
  PVR_DEPTHCMP_GREATER,
  PVR_DEPTHCMP_NOTEQUAL,
  PVR_DEPTHCMP_GEQUAL,
  PVR_DEPTHCMP_ALWAYS,
} pvr_depthcmp_mode_t;

typedef enum pvr_txr_shading_mode {
  PVR_TXRENV_REPLACE,
  PVR_TXRENV_MODULATE,
  PVR_TXRENV_DECAL,
  PVR_TXRENV_MODULATEALPHA,
} pvr_txr_shading_mode_t;

#define PVR_TXRALPHA_ENABLE 0
#define PVR_UVCLAMP_NONE 0
#define PVR_BLEND_SRCALPHA 4
#define PVR_BLEND_INVSRCALPHA 5
#define PVR_MODIFIER_OTHER_POLY 0
#define PVR_MODIFIER_INCLUDE_LAST_POLY 1
#define PVR_MODIFIER_EXCLUDE_LAST_POLY 2

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
  int specular;
  int alpha;
} pvr_context_gen_t;
typedef struct pvr_context_txr {
  int enable;
  int format;
  int width;
  int height;
  pvr_ptr_t base;
  pvr_filter_mode_t filter;
  int alpha;
  int env;
  int uv_clamp;
} pvr_context_txr_t;
typedef struct pvr_context_blend {
  int src;
  int dst;
} pvr_context_blend_t;
typedef struct pvr_context_depth {
  pvr_depthcmp_mode_t comparison;
  int write;
} pvr_context_depth_t;
typedef struct pvr_sprite_cxt {
  pvr_context_gen_t gen;
  pvr_list_t list_type;
  pvr_context_depth_t depth;
  pvr_context_txr_t txr;
} pvr_sprite_cxt_t;
typedef struct pvr_poly_cxt {
  pvr_context_gen_t gen;
  pvr_list_t list_type;
  pvr_context_txr_t txr;
  pvr_context_txr_t txr2;
  pvr_context_blend_t blend;
  pvr_context_depth_t depth;
  int modifier;
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
typedef pvr_poly_hdr_t pvr_poly_mod_hdr_t;
typedef pvr_poly_hdr_t pvr_mod_hdr_t;

typedef struct pvr_vertex {
  alignas(32) uint32_t flags;
  float x, y, z, u, v;
  uint32_t argb, oargb;
} pvr_vertex_t;

typedef struct pvr_vertex_pcm {
  alignas(32) uint32_t flags;
  float x, y, z;
  uint32_t argb0, argb1, d1, d2;
} pvr_vertex_pcm_t;

typedef struct pvr_vertex_tpcm {
  alignas(32) uint32_t flags;
  float x, y, z, u0, v0;
  uint32_t argb0, oargb0;
  float u1, v1;
  uint32_t argb1, oargb1, d1, d2, d3, d4;
} pvr_vertex_tpcm_t;

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

typedef struct pvr_modifier_vol {
  alignas(32) uint32_t flags;
  float ax, ay, az, bx, by, bz, cx, cy, cz;
  uint32_t d1, d2, d3, d4, d5, d6;
} pvr_modifier_vol_t;

#define PVR_CULLING_CCW 2
#define PVR_FOG_TABLE 0
#define PVR_SPECULAR_ENABLE 1
#define PVR_BINSIZE_0 0
#define PVR_BINSIZE_8 8
#define PVR_BINSIZE_16 16
#define PVR_BINSIZE_32 32
#define PVR_CMD_VERTEX 0xe0000000u
#define PVR_CMD_VERTEX_EOL 0xf0000000u
static inline uint32_t PVR_PACK_16BIT_UV(float u, float v) {
  union {
    float f;
    uint32_t i;
  } up = {.f = u}, vp = {.f = v};
  return (up.i & 0xffff0000u) | (vp.i >> 16u);
}
#define PC_ENDJINN_PVR_HEADER_SPRITE 0x10000000u
#define PC_ENDJINN_PVR_HEADER_DEPTH_WRITE 0x02000000u
/* Keep origin's depth-write encoding authoritative. Host-private flags occupy
 * separate unused bits for retained renderer state and volume boundaries. */
#define PC_ENDJINN_PVR_HEADER_ALPHA_CUTOUT 0x01000000u
#define PC_ENDJINN_PVR_HEADER_MODEL1_PAINTER 0x00800000u
#define PC_ENDJINN_PVR_HEADER_VOLUME_LAST 0x00400000u
#define PC_ENDJINN_PVR_HEADER_CULL_SHIFT 26u
#define PC_ENDJINN_PVR_HEADER_CULL_MASK (0x3u << PC_ENDJINN_PVR_HEADER_CULL_SHIFT)
#if ((PC_ENDJINN_PVR_HEADER_DEPTH_WRITE | \
      PC_ENDJINN_PVR_HEADER_MODEL1_PAINTER | \
      PC_ENDJINN_PVR_HEADER_ALPHA_CUTOUT | \
      PC_ENDJINN_PVR_HEADER_VOLUME_LAST) & \
     (PC_ENDJINN_PVR_HEADER_CULL_MASK | PC_ENDJINN_PVR_HEADER_SPRITE)) != 0u || \
    (PC_ENDJINN_PVR_HEADER_DEPTH_WRITE & \
     PC_ENDJINN_PVR_HEADER_MODEL1_PAINTER) != 0u || \
    (PC_ENDJINN_PVR_HEADER_DEPTH_WRITE & \
     PC_ENDJINN_PVR_HEADER_ALPHA_CUTOUT) != 0u || \
    (PC_ENDJINN_PVR_HEADER_MODEL1_PAINTER & \
     PC_ENDJINN_PVR_HEADER_ALPHA_CUTOUT) != 0u || \
    ((PC_ENDJINN_PVR_HEADER_DEPTH_WRITE | \
      PC_ENDJINN_PVR_HEADER_MODEL1_PAINTER | \
      PC_ENDJINN_PVR_HEADER_ALPHA_CUTOUT) & \
     PC_ENDJINN_PVR_HEADER_VOLUME_LAST) != 0u
#error "pc-enDjinn private PVR header flags overlap another header field"
#endif

#define PVR_TXRFMT_MIPMAP (1u << 31)
#define PVR_TXRFMT_VQ_DISABLE (0u << 30)
#define PVR_TXRFMT_VQ_ENABLE (1u << 30)
#define PVR_TXRFMT_ARGB1555 (0u << 27)
#define PVR_TXRFMT_RGB565 (1u << 27)
#define PVR_TXRFMT_ARGB4444 (2u << 27)
#define PVR_TXRFMT_YUV422 (3u << 27)
#define PVR_TXRFMT_BUMP (4u << 27)
#define PVR_TXRFMT_PAL4BPP (5u << 27)
#define PVR_TXRFMT_PAL8BPP (6u << 27)
#define PVR_TXRFMT_TWIDDLED (0u << 26)
#define PVR_TXRFMT_NONTWIDDLED (1u << 26)
#define PVR_TXRFMT_POW2_STRIDE (0u << 25)
#define PVR_TXRFMT_X32_STRIDE (1u << 25)
#define PVR_TXRFMT_8BPP_PAL(x) ((uint32_t)(x) << 25)
#define PVR_TXRFMT_4BPP_PAL(x) ((uint32_t)(x) << 21)

#define PVR_TXRLOAD_4BPP 0x01u
#define PVR_TXRLOAD_8BPP 0x02u
#define PVR_TXRLOAD_16BPP 0x03u

PC_ENDJINN_BEGIN_DECLS
uint32_t pc_endjinn_pvr_register_modifier_texture(
    const pvr_context_txr_t *texture);
void pvr_init(const pvr_init_params_t *params);
void pvr_shutdown(void);
void pvr_set_bg_color(float r, float g, float b);
void pvr_wait_ready(void);
void pvr_scene_begin(void);
void pvr_scene_finish(void);
/* pc-enDjinn-only observation hook: number of completed scene presentations.
 * It is intentionally outside the KOS pvr_* API so Dreamcast code cannot
 * acquire a dependency on it. */
uint64_t pc_endjinn_pvr_presented_frame_count(void);
void pc_endjinn_pvr_request_current_scene_screenshot(const char *path);
void pvr_list_begin(pvr_list_t list);
void pvr_list_finish(void);
void pvr_wait_render_done(void);
void pvr_set_pal_format(pvr_palfmt_t mode);
void pvr_set_pal_entry(uint32_t index, uint32_t value);
void *pvr_mem_malloc(size_t size);
void pvr_mem_free(pvr_ptr_t ptr);
void pvr_txr_load(const void *src, pvr_ptr_t dst, size_t count);
void pvr_txr_load_ex(const void *src, pvr_ptr_t dst, uint32_t width,
                     uint32_t height, uint32_t flags);
void pvr_fog_table_color(float a, float r, float g, float b);
void pvr_fog_table_linear(float start, float end);
void *pvr_dr_target(void);
void pvr_dr_commit(void *ptr);
PC_ENDJINN_END_DECLS

static inline void pvr_sprite_cxt_col(pvr_sprite_cxt_t *cxt,
                                      pvr_list_t list) {
  memset(cxt, 0, sizeof(*cxt));
  cxt->list_type = list;
  cxt->gen.culling = PVR_CULLING_CCW;
  cxt->gen.fog_type = PVR_FOG_TABLE;
  cxt->gen.specular = 0;
  cxt->depth.comparison = PVR_DEPTHCMP_GREATER;
  cxt->depth.write = 1;
}

static inline void pvr_sprite_cxt_txr(pvr_sprite_cxt_t *cxt,
                                      pvr_list_t list, int texture_format,
                                      int width, int height, pvr_ptr_t texture,
                                      pvr_filter_mode_t filter) {
  pvr_sprite_cxt_col(cxt, list);
  cxt->txr.enable = 1;
  cxt->txr.format = texture_format;
  cxt->txr.width = width;
  cxt->txr.height = height;
  cxt->txr.base = texture;
  cxt->txr.filter = filter;
}

static inline void pvr_poly_cxt_col(pvr_poly_cxt_t *cxt, pvr_list_t list) {
  memset(cxt, 0, sizeof(*cxt));
  cxt->list_type = list;
  cxt->gen.culling = PVR_CULLING_CCW;
  cxt->gen.fog_type = PVR_FOG_TABLE;
  cxt->gen.specular = 0;
  cxt->depth.comparison = PVR_DEPTHCMP_GREATER;
  cxt->depth.write = 1;
}

static inline void pvr_poly_cxt_txr(pvr_poly_cxt_t *cxt, pvr_list_t list,
                                    int texture_format, int width, int height,
                                    pvr_ptr_t texture,
                                    pvr_filter_mode_t filter) {
  pvr_poly_cxt_col(cxt, list);
  cxt->txr.enable = 1;
  cxt->txr.format = texture_format;
  cxt->txr.width = width;
  cxt->txr.height = height;
  cxt->txr.base = texture;
  cxt->txr.filter = filter;
}

static inline void pvr_poly_cxt_col_mod(pvr_poly_cxt_t *cxt,
                                        pvr_list_t list) {
  pvr_poly_cxt_col(cxt, list);
  cxt->modifier = 1;
}

static inline void pvr_poly_cxt_txr_mod(
    pvr_poly_cxt_t *cxt, pvr_list_t list, int texture_format, int width,
    int height, pvr_ptr_t texture, pvr_filter_mode_t filter,
    int texture_format2, int width2, int height2, pvr_ptr_t texture2,
    pvr_filter_mode_t filter2) {
  pvr_poly_cxt_txr(cxt, list, texture_format, width, height, texture, filter);
  cxt->modifier = 1;
  cxt->txr2.enable = 1;
  cxt->txr2.format = texture_format2;
  cxt->txr2.width = width2;
  cxt->txr2.height = height2;
  cxt->txr2.base = texture2;
  cxt->txr2.filter = filter2;
}

static inline void pvr_sprite_compile(pvr_sprite_hdr_t *hdr,
                                      const pvr_sprite_cxt_t *cxt) {
  memset(hdr, 0, sizeof(*hdr));
  hdr->cmd = 0x80000000u;
  hdr->mode1 = PC_ENDJINN_PVR_HEADER_SPRITE | (uint32_t)cxt->list_type |
      (cxt->txr.enable ? 0x80000000u : 0u) |
      (cxt->depth.write ? PC_ENDJINN_PVR_HEADER_DEPTH_WRITE : 0u) |
      (cxt->gen.alpha ? PC_ENDJINN_PVR_HEADER_ALPHA_CUTOUT : 0u) |
      (((uint32_t)cxt->gen.culling & 0x3u) << PC_ENDJINN_PVR_HEADER_CULL_SHIFT);
  hdr->mode2 = (uint32_t)cxt->txr.format;
  hdr->mode3 = (uint32_t)cxt->txr.width | ((uint32_t)cxt->txr.height << 16u);
  hdr->argb = 0xffffffffu;
  hdr->oargb = (uint32_t)cxt->txr.filter;
  const uintptr_t base = (uintptr_t)cxt->txr.base;
  hdr->reserved[0] = (uint32_t)base;
#if UINTPTR_MAX > UINT32_MAX
  hdr->reserved[1] = (uint32_t)(base >> 32u);
#endif
}

static inline void pvr_poly_compile(pvr_poly_hdr_t *hdr,
                                    const pvr_poly_cxt_t *cxt) {
  pvr_sprite_cxt_t sprite = {.gen = cxt->gen,
                             .list_type = cxt->list_type,
                             .depth = cxt->depth,
                             .txr = cxt->txr};
  pvr_sprite_compile(hdr, &sprite);
  hdr->mode1 &= ~PC_ENDJINN_PVR_HEADER_SPRITE;
}

static inline void pvr_poly_mod_compile(pvr_poly_mod_hdr_t *hdr,
                                        const pvr_poly_cxt_t *cxt) {
  pvr_poly_compile(hdr, cxt);
  hdr->mode1 |= cxt->modifier ? 0x40000000u : 0u;
  if (cxt->modifier && cxt->txr2.enable) {
    hdr->cmd |= pc_endjinn_pvr_register_modifier_texture(&cxt->txr2) &
                0x0fffffffu;
  }
}

static inline void pvr_mod_compile(pvr_mod_hdr_t *hdr, pvr_list_t list,
                                   uint32_t mode, uint32_t culling) {
  memset(hdr, 0, sizeof(*hdr));
  hdr->cmd = 0x80000000u;
  hdr->mode1 = (uint32_t)list | 0x20000000u |
      ((culling & 0x3u) << PC_ENDJINN_PVR_HEADER_CULL_SHIFT) |
      (mode == PVR_MODIFIER_INCLUDE_LAST_POLY ||
               mode == PVR_MODIFIER_EXCLUDE_LAST_POLY
           ? PC_ENDJINN_PVR_HEADER_VOLUME_LAST
           : 0u);
  hdr->oargb = mode;
  hdr->argb = 0xffffffffu;
}

#endif
