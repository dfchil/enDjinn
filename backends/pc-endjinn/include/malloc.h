#ifndef PC_ENDJINN_MALLOC_H
#define PC_ENDJINN_MALLOC_H

#include <stdlib.h>

#ifndef memalign
void *memalign(size_t alignment, size_t size);
#endif

#endif
