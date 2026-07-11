#ifndef PC_ENDJINN_KOS_H
#define PC_ENDJINN_KOS_H

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include <arch/timer.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/purupuru.h>
#include <dc/maple/vmu.h>
#include <dc/perf_monitor.h>
#include <dc/pvr.h>
#include <dc/sound/sfxmgr.h>
#include <dc/sound/sound.h>
#include <dc/video.h>

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#define INIT_DEFAULT 0u
#define KOS_INIT_FLAGS(flags)
#define ARCH_EXIT_REBOOT 3

PC_ENDJINN_BEGIN_DECLS
void arch_set_exit_path(int path);
PC_ENDJINN_END_DECLS

#endif
