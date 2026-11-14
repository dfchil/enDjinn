#ifndef PC_ENDJINN_KOS_THREAD_H
#define PC_ENDJINN_KOS_THREAD_H

#include <pc_endjinn/types.h>

PC_ENDJINN_BEGIN_DECLS
/* Yields the processor. */
void thd_pass(void);
/* Sleeps for at least milliseconds. In a browser this waits without
 * returning to the event loop, so audio and input stop meanwhile. */
void thd_sleep(unsigned int milliseconds);
PC_ENDJINN_END_DECLS

#endif
