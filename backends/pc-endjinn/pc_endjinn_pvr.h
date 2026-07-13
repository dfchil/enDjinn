#ifndef PC_ENDJINN_PVR_H
#define PC_ENDJINN_PVR_H

#include <kos.h>

#include <vector>

namespace pc_endjinn_pvr {

struct QueuedPrimitive {
  float x[4];
  float y[4];
  float z[4];
  uint32_t count;
  uint32_t argb;
  pvr_list_t list;
};

const std::vector<QueuedPrimitive> &primitives();
void scene_begin();
void list_begin(pvr_list_t list);
void *dr_target();
void dr_commit(void *ptr);

}  // namespace pc_endjinn_pvr

#endif
