#include <assert.h>
#include <enDjinn/enj_draw.h>

static void **_dr64_1st_half = NULL;
static void **_dr64_2nd_half = NULL;

void enj_draw_pvr_dr64_reset(void) {
  *_dr64_1st_half = pvr_dr_target();
  *_dr64_2nd_half = NULL;
}

void enj_draw_pvr_dr64_init(void **first_half, void **second_half) {
  _dr64_1st_half = first_half;
  _dr64_2nd_half = second_half;
  enj_draw_pvr_dr64_reset();
}

void enj_draw_pvr_dr64_commit_1st(void) {
  pvr_dr_commit(*_dr64_1st_half);
  *_dr64_2nd_half = (void *)(((int)pvr_dr_target()) - 32);
}

void enj_draw_pvr_dr64_commit_2nd(void) {
  pvr_dr_commit((void *)(((int)(*_dr64_2nd_half)) + 32));
}

void enj_draw_sprite(float corners[4][3], pvr_sprite_hdr_t *hdr,
                     uint32_t UVs[3]) {
  // skipping the header is ok if a header was committed outside of this
  // function
  if (hdr != NULL) {
    pvr_sprite_hdr_t *mode_hdr = (pvr_sprite_hdr_t *)pvr_dr_target();
    *mode_hdr = *hdr;
    pvr_dr_commit(mode_hdr);
  }
  pvr_sprite_txr_t *quad_1sthalf, *quad_2ndhalf;
  enj_draw_pvr_dr64_init((void **)&quad_1sthalf, (void **)&quad_2ndhalf);

  quad_1sthalf->flags = PVR_CMD_VERTEX_EOL;
  quad_1sthalf->ax = corners[0][0];
  quad_1sthalf->ay = corners[0][1];
  quad_1sthalf->az = corners[0][2];
  quad_1sthalf->bx = corners[1][0];
  quad_1sthalf->by = corners[1][1];
  quad_1sthalf->bz = corners[1][2];
  quad_1sthalf->cx = corners[2][0];
  enj_draw_pvr_dr64_commit_1st();
  quad_2ndhalf->cy = corners[2][1];
  quad_2ndhalf->cz = corners[2][2];
  quad_2ndhalf->dx = corners[3][0];
  quad_2ndhalf->dy = corners[3][1];
  if (UVs) {
    quad_2ndhalf->auv = UVs[0];
    quad_2ndhalf->buv = UVs[1];
    quad_2ndhalf->cuv = UVs[2];
  } else {
    quad_2ndhalf->auv = PVR_PACK_16BIT_UV(0.0f, 1.0f);
    quad_2ndhalf->buv = PVR_PACK_16BIT_UV(0.0f, 0.0f);
    quad_2ndhalf->cuv = PVR_PACK_16BIT_UV(1.0f, 0.0f);
  }
  enj_draw_pvr_dr64_commit_2nd();
}