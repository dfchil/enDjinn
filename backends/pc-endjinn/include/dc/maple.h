#ifndef PC_ENDJINN_DC_MAPLE_H
#define PC_ENDJINN_DC_MAPLE_H

#include <pc_endjinn/types.h>

#define MAPLE_PORT_COUNT 4
#define MAPLE_UNIT_COUNT 6
#define MAPLE_FUNC_CONTROLLER 0x01000000u
#define MAPLE_FUNC_LCD 0x04000000u
#define MAPLE_FUNC_PURUPURU 0x00010000u

typedef struct maple_device {
  int port;
  int unit;
  bool valid;
  struct {
    uint32_t functions;
  } info;
} maple_device_t;

typedef void (*maple_attach_callback_t)(maple_device_t *device);
typedef void (*maple_detach_callback_t)(maple_device_t *device);

PC_ENDJINN_BEGIN_DECLS
maple_device_t *maple_enum_type(int index, uint32_t function);
maple_device_t *maple_enum_dev(int port, int unit);
void *maple_dev_status(maple_device_t *device);
void maple_attach_callback(uint32_t function, maple_attach_callback_t callback);
void maple_detach_callback(uint32_t function, maple_detach_callback_t callback);
PC_ENDJINN_END_DECLS

#endif
