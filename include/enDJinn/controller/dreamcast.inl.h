#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/vmu.h>
#include <enDJinn/controller/abstract_ctrlr.h>
#include <enDJinn/controller/dreamcast.h>
#include <enDJinn/compiledefs.h>
#include <stddef.h>

static cont_state_t state_backups[MAPLE_PORT_COUNT] = {0};

static controller_state_t ctrlr_states_storage[MAPLE_PORT_COUNT] = {0};
static controller_state_t *ctrlr_states_refs[MAPLE_PORT_COUNT] = {0};

int dc_ctrlrs_map_state(void) {
  int count = 0;
  for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
    maple_device_t *device = maple_port_type(i, MAPLE_FUNC_CONTROLLER);

    if (device && device->valid == true) {
      ctrlr_states_refs[i] = ctrlr_states_storage + i;
      cont_state_t *state = (cont_state_t *)maple_dev_status(device);
      state_backups[i] = *state;
      ctrlr_states_refs[i]->state = state_backups + i;
      if (state) {
        // Map the cont_state_t onto controller_state_t
        cont_state_onto_ctrlstate(state, ctrlr_states_refs[i]);
        ctrlr_states_refs[i]->portnum = i;
        count++;
      }
    } else {
      ctrlr_states_refs[i] = NULL;
    }
  }
  return count;
}

/* Return the first device of the requested type on port p */
maple_device_t *maple_port_type(int p, uint32 func) {
  maple_device_t *dev;
  for (int u = 0; u < MAPLE_UNIT_COUNT; u++) {
    dev = maple_enum_dev(p, u);
    if (dev != NULL && (dev->info.functions & func)) {
      return dev;
    }
  }
  return NULL;
}

// maple_device_t **get_ctrlr_devices(void) { return ctrl_state_refs; }

controller_state_t **get_ctrlr_states(void) { return ctrlr_states_refs; }