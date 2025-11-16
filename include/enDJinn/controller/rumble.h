#ifndef PURUPURU_H
#define PURUPURU_H

#include <dc/maple/purupuru.h>
#include <stdint.h>

static const uint32_t catalog[] = {
    0x011A7010, // Basic Thud (simple .5s jolt)
    0x00072010, // Ship's Thrust (as in AAC)
    0x31071011, // "Car Idle (69 Mustang)
    0x4539c010, // disintegration
};

void print_rumble_fields(purupuru_effect_t fields);

void rumble_queued(uint32_t ctrloffset, uint32_t raw);
void rumble_queues_init(void);
void rumble_queues_shutdown(void);

#endif // PURUPURU_H