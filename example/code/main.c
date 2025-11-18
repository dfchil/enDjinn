#include <kos.h>
#ifdef DCPROF
#include "../profilers/dcprof/profiler.h"
#endif

KOS_INIT_FLAGS(INIT_DEFAULT);

#ifdef DEBUG
#include <arch/gdb.h>
#endif


#ifdef DEBUG
#include <dc/perf_monitor.h>
#endif

#include <dc/pvr.h>
#include <enDjinn/core.h>

static pvr_init_params_t pvr_params = {
    {PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16,
     PVR_BINSIZE_0},
    1024 * 1024,   // Vertex buffer size
    0,             // No DMA
    0, // Set horisontal FSAA
    0,             // Translucent Autosort enabled.
    3,             // Extra OPBs
    0,             // No extra PTs
};

int main(__unused int argc, __unused char **argv) {
#ifdef DEBUG
  gdb_init();
  DEBUG_PRINT("CBASEPATH %s\n", CBASEPATH);

  perf_monitor_init(PMCR_OPERAND_CACHE_READ_MISS_MODE,
                    PMCR_INSTRUCTION_CACHE_MISS_MODE);
#endif

  vid_set_mode(DM_640x480, PM_RGB888P);
  pvr_set_bg_color(0.0, 0.0, 24.0f / 255.0f);
  pvr_init(&pvr_params);

  if (!core_init()) {
    // DEBUG_PRINT("Core init failed, exiting\n");
    return -1;
  };

#ifdef DCPROF
  profiler_init("/pc/gmon.out");
  profiler_start();
#endif

  enj_run();

#ifdef DCPROF
  profiler_stop();
  profiler_clean_up();
#endif
  // rumble_queues_shutdown();
  pvr_shutdown();

#ifdef DEBUG
  perf_monitor_print(stdout);

  FILE *stats_out = fopen(CBASEPATH "/pstats.txt", "a");
  if (stats_out != NULL) {
    perf_monitor_print(stats_out);
    fclose(stats_out);
  }
#endif

#ifdef RELEASEBUILD
  arch_set_exit_path(ARCH_EXIT_REBOOT);
#endif
  arch_exit();
}
