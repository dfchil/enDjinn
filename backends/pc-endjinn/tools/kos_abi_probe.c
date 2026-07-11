#include <stddef.h>
#include <kos.h>

#define ABI_VALUE(name, expression)                                             \
  void pc_endjinn_abi_##name(void) {                                            \
    __asm__ volatile(".ascii \"PC_ENDJINN_ABI " #name " %c0\\n\""              \
                     : : "i"(expression));                                     \
  }

ABI_VALUE(SIZEOF_PVR_INIT_PARAMS, sizeof(pvr_init_params_t))
ABI_VALUE(ALIGNOF_PVR_INIT_PARAMS, _Alignof(pvr_init_params_t))
ABI_VALUE(OFFSETOF_PVR_INIT_PARAMS_VERTEX_BUF_SIZE,
          offsetof(pvr_init_params_t, vertex_buf_size))
ABI_VALUE(OFFSETOF_PVR_INIT_PARAMS_OPB_SIZES,
          offsetof(pvr_init_params_t, opb_sizes))

ABI_VALUE(SIZEOF_PVR_VERTEX, sizeof(pvr_vertex_t))
ABI_VALUE(ALIGNOF_PVR_VERTEX, _Alignof(pvr_vertex_t))
ABI_VALUE(OFFSETOF_PVR_VERTEX_X, offsetof(pvr_vertex_t, x))
ABI_VALUE(OFFSETOF_PVR_VERTEX_U, offsetof(pvr_vertex_t, u))
ABI_VALUE(OFFSETOF_PVR_VERTEX_ARGB, offsetof(pvr_vertex_t, argb))

ABI_VALUE(SIZEOF_PVR_SPRITE_HDR, sizeof(pvr_sprite_hdr_t))
ABI_VALUE(ALIGNOF_PVR_SPRITE_HDR, _Alignof(pvr_sprite_hdr_t))
ABI_VALUE(OFFSETOF_PVR_SPRITE_HDR_ARGB, offsetof(pvr_sprite_hdr_t, argb))

ABI_VALUE(SIZEOF_PVR_SPRITE_COL, sizeof(pvr_sprite_col_t))
ABI_VALUE(ALIGNOF_PVR_SPRITE_COL, _Alignof(pvr_sprite_col_t))
ABI_VALUE(OFFSETOF_PVR_SPRITE_COL_DX, offsetof(pvr_sprite_col_t, dx))

ABI_VALUE(SIZEOF_PVR_SPRITE_TXR, sizeof(pvr_sprite_txr_t))
ABI_VALUE(ALIGNOF_PVR_SPRITE_TXR, _Alignof(pvr_sprite_txr_t))
ABI_VALUE(OFFSETOF_PVR_SPRITE_TXR_AUV, offsetof(pvr_sprite_txr_t, auv))

ABI_VALUE(SIZEOF_CONT_STATE, sizeof(cont_state_t))
ABI_VALUE(ALIGNOF_CONT_STATE, _Alignof(cont_state_t))
ABI_VALUE(OFFSETOF_CONT_STATE_BUTTONS, offsetof(cont_state_t, buttons))
ABI_VALUE(OFFSETOF_CONT_STATE_LTRIG, offsetof(cont_state_t, ltrig))
ABI_VALUE(OFFSETOF_CONT_STATE_JOYX, offsetof(cont_state_t, joyx))

ABI_VALUE(SIZEOF_PURUPURU_EFFECT, sizeof(purupuru_effect_t))
ABI_VALUE(ALIGNOF_PURUPURU_EFFECT, _Alignof(purupuru_effect_t))

ABI_VALUE(PVR_LIST_OP_POLY, PVR_LIST_OP_POLY)
ABI_VALUE(PVR_LIST_TR_POLY, PVR_LIST_TR_POLY)
ABI_VALUE(PVR_LIST_PT_POLY, PVR_LIST_PT_POLY)
ABI_VALUE(PVR_CMD_VERTEX, PVR_CMD_VERTEX)
ABI_VALUE(PVR_CMD_VERTEX_EOL, PVR_CMD_VERTEX_EOL)
ABI_VALUE(MAPLE_PORT_COUNT, MAPLE_PORT_COUNT)
ABI_VALUE(MAPLE_UNIT_COUNT, MAPLE_UNIT_COUNT)
ABI_VALUE(MAPLE_FUNC_CONTROLLER, MAPLE_FUNC_CONTROLLER)
ABI_VALUE(MAPLE_FUNC_LCD, MAPLE_FUNC_LCD)
ABI_VALUE(MAPLE_FUNC_PURUPURU, MAPLE_FUNC_PURUPURU)
ABI_VALUE(CONT_A, CONT_A)
ABI_VALUE(CONT_B, CONT_B)
ABI_VALUE(CONT_X, CONT_X)
ABI_VALUE(CONT_Y, CONT_Y)
ABI_VALUE(CONT_START, CONT_START)
