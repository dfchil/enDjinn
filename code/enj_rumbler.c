

#include <enDjinn/enj_rumbler.h>
#include <dc/maple/purupuru.h>


static int enj_rumble_rate_limit = 6; // frames of cooldown between rumble commands

static maple_device_t* local_rumblers[MAPLE_PORT_COUNT] = {0};
static int rumble_rate_limits[MAPLE_PORT_COUNT] = {0};
static uint32_t pending_rumble_effects[MAPLE_PORT_COUNT] = {0};

/* Called at init and then only as a callback when purupuru
   devices are connected or disconnected */
void scan_local_rumblers(maple_device_t* __unused) {
    /* Clear existing rumble info */
    for (int i = 0; i < 4; i++) {
        local_rumblers[i] = NULL;
    }

    /* Loop through all available purupuru packs
       and assign them to the proper ports  */
    int i = 0;
    maple_device_t* purupuru;
    while ((purupuru = maple_enum_type(i, MAPLE_FUNC_PURUPURU))) {
        local_rumblers[purupuru->port] = purupuru;
        i++;
    }
}

void enj_rumble_init_local_devices(void) {
    scan_local_rumblers(NULL);
    maple_attach_callback(MAPLE_FUNC_PURUPURU, scan_local_rumblers);
    maple_detach_callback(MAPLE_FUNC_PURUPURU, scan_local_rumblers);
}

void enj_rumble_set_rate_limit(int frames) {
    enj_rumble_rate_limit = frames;
}

enj_rumbler_reply_e enj_rumbler_set(enj_ctrl_port_name_e ctrloffset,
                                   uint32_t raw) {
    
    if (ctrloffset > MAPLE_PORT_COUNT) {
      return enj_rumble_no_device;
    } 
    maple_device_t* rumble_dev  = local_rumblers[ctrloffset];
    if (rumble_dev == NULL) {
        return enj_rumble_no_device;
    }
    enj_rumbler_reply_e reply = enj_rumble_set;
    if (pending_rumble_effects[ctrloffset] != 0) {
        // there's a pending effect to send
        reply |= enj_rumble_overwrote_previous;
    }
    if (rumble_rate_limits[ctrloffset] > 0) {
        reply |= enj_rumble_rate_limited;
    } else {
      rumble_rate_limits[ctrloffset] = enj_rumble_rate_limit;
    }

    pending_rumble_effects[ctrloffset] = raw;
    return reply;
}