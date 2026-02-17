#ifndef ENJ_DRAW_H
#define ENJ_DRAW_H
#include <dc/pvr.h>

/** Initialize the store queue system for 64 byte PVR structs, like
 * sprites and modifier volumes headers.
 * @param first_half Pointer to be used for the first half of the struct
 * @param second_half Pointer to be used for the second half of the struct
 */
void enj_draw_pvr_dr64_init(void **first_half, void **second_half);

/** Commit the first half of the DR64 drawing system
 *
 * @note This should be called after filling in the first half of the struct,
 * and before filling in the second half.
 *
 * @warning The pointer to the first half will be set to NULL after committing.
 */
void enj_draw_pvr_dr64_commit_1st(void);

/** Commit the second half of the DR64 drawing system
 * @note This should be called after calling enj_draw_pvr_dr64_commit_1st and
 * after filling in the second half of the struct
 */
void enj_draw_pvr_dr64_commit_2nd(void);

/** Reset the DR64 drawing system */
void enj_draw_pvr_dr64_reset(void);

/**
 * Draw a sprite with given corners, PVR state, header and UVs
 * @param corners Array of 4 corners, each with 3 floats (x, y, z)
 * @param state_ptr Pointer to PVR drawing state
 * @param hdr Optional pointer to PVR sprite header
 * @param UVs Array of 3 UV coordinates packed as uint32_t
 */
void enj_draw_sprite(float corners[4][3], pvr_sprite_hdr_t *hdr,
                     uint32_t UVs[3]);

#endif /* ENJ_DRAW_H */