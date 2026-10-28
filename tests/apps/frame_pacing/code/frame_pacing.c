#include <enDjinn/enj_enDjinn.h>

#include <stdio.h>

/* Runs each phase for PHASE_TICKS ticks under one pacing and a number of
 * full-screen translucent layers, prints one FRAMEPACING line per phase and
 * exits. On a Dreamcast four layers fit a refresh and five do not;
 * tests/frame_pacing_hw.py checks the lines. */

#define PHASE_TICKS 360

static const struct {
  enj_pacing_t pacing;
  int layers;
} phases[] = {
    {ENJ_PACING_STRICT, 0},    {ENJ_PACING_DROP_LATE, 0},
    {ENJ_PACING_STRICT, 4},    {ENJ_PACING_DROP_LATE, 4},
    {ENJ_PACING_STRICT, 5},    {ENJ_PACING_DROP_LATE, 5},
    {ENJ_PACING_HALF, 5},      {ENJ_PACING_STRICT, 6},
    {ENJ_PACING_DROP_LATE, 6},
};
#define PHASE_COUNT ((int)(sizeof(phases) / sizeof(phases[0])))

static const char *const pacing_names[] = {"FREE", "STRICT", "DROP_LATE",
                                           "HALF"};

static pvr_sprite_hdr_t layer_hdr;
static int phase = -1;
static uint32_t phase_ticks;
static uint64_t phase_start_ns;
static enj_pacing_stats_t phase_start;

static void render_layers(void *__unused data) {
  const float w = 640.0f * ENJ_XSCALE;
  for (int i = 0; i < phases[phase].layers; i++) {
    float z = 1.0f + (float)i;
    float corners[4][3] = {{0.0f, 0.0f, z}, {w, 0.0f, z}, {w, 480.0f, z},
                           {0.0f, 480.0f, z}};
    enj_draw_sprite(corners, &layer_hdr, NULL);
  }
}

static void start_phase(void) {
  enj_pacing_set(phases[phase].pacing);
  enj_pacing_stats(&phase_start);
  phase_start_ns = timer_ns_gettime64();
  phase_ticks = 0;
}

static void end_phase(void) {
  enj_pacing_stats_t now;
  enj_pacing_stats(&now);
  printf("FRAMEPACING pacing=%s layers=%d ticks=%lu rendered=%lu dropped=%lu "
         "late=%lu ms=%lu\n",
         pacing_names[phases[phase].pacing], phases[phase].layers,
         (unsigned long)(now.ticks - phase_start.ticks),
         (unsigned long)(now.rendered - phase_start.rendered),
         (unsigned long)(now.dropped - phase_start.dropped),
         (unsigned long)(now.late - phase_start.late),
         (unsigned long)((timer_ns_gettime64() - phase_start_ns) / 1000000ull));
}

static void frame_pacing_updater(void *__unused data) {
  /* The counters of the tick that ends a phase belong to the next one: the
   * phase switches before this tick is counted. */
  if (phase < 0 || ++phase_ticks >= PHASE_TICKS) {
    if (phase >= 0) {
      end_phase();
    }
    if (++phase >= PHASE_COUNT) {
      enj_state_flag_shutdown(NULL);
      return;
    }
    start_phase();
  }
  if (!enj_pacing_scene_dropped()) {
    enj_render_list_add(PVR_LIST_TR_POLY, render_layers, NULL);
  }
}

int main(__unused int argc, __unused char **argv) {
  enj_state_init_defaults();
  if (enj_state_startup() != 0) {
    return -1;
  }
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_col(&cxt, PVR_LIST_TR_POLY);
  pvr_sprite_compile(&layer_hdr, &cxt);
  layer_hdr.argb = 0x10406080;

  enj_mode_t mode = {
      .name = "Frame pacing",
      .mode_updater = frame_pacing_updater,
  };
  enj_mode_push(&mode);
  enj_state_run();
  return 0;
}
