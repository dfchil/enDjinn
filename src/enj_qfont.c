
#define ENJ_INJECT_QFONT

#ifdef ENJ_INJECT_QFONT
#include <enDjinn/embeds/enj_qfont_data.h>
#include <enDjinn/enj_draw.h>
#include <enDjinn/enj_font.h>
#include <enDjinn/enj_qfont.h>
#include <enDjinn/enj_render.h>
static enj_font_header_t* enj_qf_hdr;
static pvr_ptr_t enj_qf_pvr_data;
static pvr_sprite_hdr_t enj_qf_sprite_hdr;
static pvr_list_type_t enj_qf_prev_mode;

static inline void enj_qfont_set_header(pvr_list_type_t mode) {
    pvr_sprite_cxt_t cxt;

    pvr_sprite_cxt_txr(&cxt, mode, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED,
                       1 << enj_qf_hdr->log2width, 1 << enj_qf_hdr->log2height,
                       (pvr_ptr_t)enj_qf_pvr_data,
                       PVR_FILTER_NEAREST);
    pvr_sprite_compile(&enj_qf_sprite_hdr, &cxt);
    enj_qf_prev_mode = mode;
}

int enj_qfont_init() {

    enj_qf_hdr = (enj_font_header_t*)enj_qfont_data;

    enj_qf_pvr_data = enj_font_to_16bit_texture(
        enj_qf_hdr, enj_qfont_data + sizeof(enj_font_header_t), PVR_PIXEL_MODE_ARGB1555,
        (enj_color_t){.raw = 0xffffffff}, (enj_color_t){.raw = 0x00000000});

    enj_qfont_set_header(PVR_LIST_PT_POLY);
    return 0;
}

int enj_qfont_write(const char* str, int x, int y, pvr_list_type_t cur_mode) {
    pvr_dr_state_t state;
    pvr_dr_init(&state);

    if (cur_mode != enj_qf_prev_mode) {
        // rewrite the header if the mode has changed
        enj_qf_prev_mode = cur_mode;
        enj_qfont_set_header(cur_mode);
    }

    int renderwidth = enj_font_string_render(str, enj_qf_hdr, x, y,
                                       &enj_qf_sprite_hdr, &state);
    ENJ_DEBUG_PRINT("%d\n", enj_font_render_glyph('H', enj_qf_hdr, x, y, &state)); 
    pvr_dr_finish();
    return renderwidth;
}
void enj_debug_shutdown() {
    if (enj_qf_pvr_data){
        pvr_mem_free(enj_qf_pvr_data);
    }
    enj_qf_pvr_data = NULL;
}

#endif  // ENJ_INJECT_QFONT
