

#include <enDjinn/enj_rumble.h>
#include <dc/maple/purupuru.h>
#include <enDjinn/enj_defs.h>

static int enj_rumble_rate_limit = 1; // frames of cooldown between rumble commands
static maple_device_t* local_rumbles[MAPLE_PORT_COUNT] = {0};
static int rumble_rate_limits[MAPLE_PORT_COUNT] = {0};
static uint32_t pending_rumble_effects[MAPLE_PORT_COUNT] = {0};

/* Called at init and then only as a callback when purupuru
   devices are connected or disconnected */
static inline void scan_local_rumblers(maple_device_t* __unused) {
    /* Clear existing rumble info */
    for (int i = 0; i < 4; i++) {
        local_rumbles[i] = NULL;
    }

    /* Loop through all available purupuru packs
       and assign them to the proper ports  */
    int i = 0;
    maple_device_t* purupuru;
    while ((purupuru = maple_enum_type(i, MAPLE_FUNC_PURUPURU))) {
        local_rumbles[purupuru->port] = purupuru;
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

enj_rumble_reply_e enj_rumble_set_effect(enj_ctrl_port_name_e ctrloffset,
                                   uint32_t raw) {
    
    if (ctrloffset > MAPLE_PORT_COUNT) {
      return enj_rumble_no_device;
    } 
    maple_device_t* rumble_dev  = local_rumbles[ctrloffset];
    if (rumble_dev == NULL) {
        return enj_rumble_no_device;
    }
    enj_rumble_reply_e reply = enj_rumble_set;
    if (pending_rumble_effects[ctrloffset] != 0) {
        // there's a pending effect to send
        reply |= enj_rumble_overwrote_previous;
    }
    if (rumble_rate_limits[ctrloffset] > 0) {
        reply |= enj_rumble_rate_limited;
    }

    pending_rumble_effects[ctrloffset] = raw;
    return reply;
}

maple_device_t** enj_rumble_get_states() {
    return local_rumbles;
}

void enj_rumble_update(void) {
    for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
        if (rumble_rate_limits[i] > 0) {
            rumble_rate_limits[i]--;
        }
        if (local_rumbles[i] == NULL) {
            // no rumble device on this port
            if (pending_rumble_effects[i] != 0 || rumble_rate_limits[i] != 0) {
                ENJ_DEBUG_PRINT(
                    "Rumble effect pending for port %d but no device!\n", i);
            }
            pending_rumble_effects[i] = 0;
            rumble_rate_limits[i] = 0;
            continue;
        }
        if (pending_rumble_effects[i] != 0 && rumble_rate_limits[i] == 0) {
            purupuru_rumble_raw(local_rumbles[i],
                                  pending_rumble_effects[i]);
            pending_rumble_effects[i] = 0;
            rumble_rate_limits[i] = enj_rumble_rate_limit;
        }
    }
}