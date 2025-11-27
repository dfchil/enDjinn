#include <enDjinn/enj_draw.h>

void enj_draw_sprite(float corners[4][3], pvr_dr_state_t *state_ptr,
                     pvr_sprite_hdr_t *hdr, uint32_t UVs[3]) {
  if (state_ptr == NULL) {
    static pvr_dr_state_t static_dr_state;
    pvr_dr_init(&static_dr_state);
    state_ptr = &static_dr_state;
  }
  // skipping the header is valid if header was sent previously
  if (hdr != NULL) {
    pvr_sprite_hdr_t *mode_hdr =
        (pvr_sprite_hdr_t *)pvr_dr_target(*state_ptr);
    *mode_hdr = *hdr;
    pvr_dr_commit(mode_hdr);
  }
  pvr_sprite_txr_t *quad = (pvr_sprite_txr_t *)pvr_dr_target(*state_ptr);
  quad->flags = PVR_CMD_VERTEX_EOL;
  quad->ax = corners[0][0];
  quad->ay = corners[0][1];
  quad->az = corners[0][2];
  quad->bx = corners[1][0];
  quad->by = corners[1][1];
  quad->bz = corners[1][2];
  quad->cx = corners[2][0];
  pvr_dr_commit(quad);
  quad = (pvr_sprite_txr_t *)pvr_dr_target(*state_ptr);
  pvr_sprite_txr_t *quad2ndhalf = (pvr_sprite_txr_t *)((int)quad - 32);
  quad2ndhalf->cy = corners[2][1];
  quad2ndhalf->cz = corners[2][2];
  quad2ndhalf->dx = corners[3][0];
  quad2ndhalf->dy = corners[3][1];
  if (UVs) {
    quad2ndhalf->auv = UVs[0];
    quad2ndhalf->buv = UVs[1];
    quad2ndhalf->cuv = UVs[2];
  } else {
    quad2ndhalf->auv = PVR_PACK_16BIT_UV(0.0f, 0.0f);
    quad2ndhalf->buv = PVR_PACK_16BIT_UV(1.0f, 0.0f);
    quad2ndhalf->cuv = PVR_PACK_16BIT_UV(1.0f, 1.0f);
  }
  pvr_dr_commit(quad);
  pvr_dr_finish();
}
