#ifndef PC_ENDJINN_RETAIN_H
#define PC_ENDJINN_RETAIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Retained scene ranges. A game that presents one simulation step on more
 * than one display refresh can keep the primitives one renderer queued and
 * queue the same primitives again on the following scene, instead of running
 * that renderer twice. Primitives keep their list, header state and texture
 * handles; textures must stay allocated while a slot refers to them. */
#define ENJ_PC_RETAIN_SLOTS 4u

/* Primitives queued so far in the current scene. */
size_t enj_pc_queued_primitive_count(void);

/* Copy the primitives queued since `first` into `slot`, replacing what the
 * slot held. Returns false for an invalid slot or range. */
bool enj_pc_retain_primitives(uint32_t slot, size_t first);

/* Queue a copy of the slot's primitives into the current scene, after what
 * is already queued. Returns false for an invalid or empty slot. */
bool enj_pc_replay_primitives(uint32_t slot);

/* Forget what a slot holds. */
void enj_pc_clear_retained_primitives(uint32_t slot);

#ifdef __cplusplus
}
#endif

#endif
