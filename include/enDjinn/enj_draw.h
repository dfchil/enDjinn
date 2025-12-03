#include <dc/pvr.h>

/**
 * Draw a sprite with given corners, PVR state, header and UVs
 * @param corners Array of 4 corners, each with 3 floats (x, y, z)
 * @param state_ptr Pointer to PVR drawing state
 * @param hdr Optional pointer to PVR sprite header
 * @param UVs Array of 3 UV coordinates packed as uint32_t
 */
void enj_draw_sprite(float corners[4][3], pvr_dr_state_t *state_ptr,
                     pvr_sprite_hdr_t *hdr, uint32_t UVs[3]);