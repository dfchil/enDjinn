#ifndef ENJ_VMU_H
#define ENJ_VMU_H

/*! \file
 *  \brief   Save files on the VMU.
 *
 *  One save per application, kept in the application's own memory and stored
 *  on the first VMU as a VMS file with a description and an icon. Load it at
 *  boot, then either write it when you like or start the background saver,
 *  which writes it shortly after it stops changing.
 *
 *  \author    2026 Daniel Fairchild
 *  \copyright MIT License
 */

#include <enDjinn/enj_api.h>

#include <stddef.h>
#include <stdint.h>

ENJ_BEGIN_DECLS

/** What a save is and how the VMU file menus show it. */
typedef struct enj_vmu_save {
  /** File name on the VMU, at most 12 characters. */
  const char *name;
  /** Shown in the VMU's own file menu, at most 16 characters. */
  const char *desc_short;
  /** Shown in the Dreamcast file manager, at most 32 characters. */
  const char *desc_long;
  /** Application id, at most 16 characters. */
  const char *app_id;
  /** 16 ARGB4444 colours, or NULL for a save with no icon. */
  const uint16_t *icon_palette;
  /** 512 bytes per frame: 32x32 pixels, 4 bits each, the left one high. */
  const uint8_t *icon_frames;
  /** 1 to 3 frames. */
  int icon_frame_count;
  /** Frames of the Dreamcast's 1/30 s between icon frames. */
  int icon_anim_speed;
  /** The save, in the application's memory. */
  void *data;
  /** Its size in bytes. */
  size_t size;
} enj_vmu_save_t;

typedef enum {
  /** The save was read into save->data. */
  ENJ_VMU_LOADED = 0,
  /** A VMU, but no save on it: the first write goes there. */
  ENJ_VMU_NO_SAVE,
  /** No VMU is plugged in. */
  ENJ_VMU_NO_DEVICE,
  /** A file by that name that is not this save (wrong size or bad CRC). */
  ENJ_VMU_UNREADABLE,
} enj_vmu_load_e;

/**
 * Reads the save from the first VMU into save->data. save->data is left as it
 * was unless ENJ_VMU_LOADED is returned. Blocks for the maple transfers.
 */
enj_vmu_load_e enj_vmu_save_load(const enj_vmu_save_t *save);

/**
 * Writes save->data to the VMU it was loaded from, or to the first VMU.
 * Blocks for the maple transfers. Returns 0, or -1 if there is no VMU or the
 * write failed.
 */
int enj_vmu_save_write(const enj_vmu_save_t *save);

/**
 * Starts the background saver for save, which must stay valid. After
 * enj_vmu_save_changed() it waits until there have been no changes for
 * delay_ms, then writes a copy of save->data from a low-priority thread, so
 * the game thread never waits for the VMU. A failed write is tried again five
 * seconds later. A VMU plugged in after enj_vmu_save_load() that already holds
 * a save is not written over. Calling it again does nothing.
 */
void enj_vmu_save_start(const enj_vmu_save_t *save, unsigned delay_ms);

/** Hold around changes to save->data, so the saver never copies half of one. */
void enj_vmu_save_lock(void);
void enj_vmu_save_unlock(void);

/** Tells the background saver that save->data has changed. */
void enj_vmu_save_changed(void);

/**
 * For a reset or quit: if changes are waiting, the saver writes them at once
 * and then calls arch_exit(). Returns 1 if it will, 0 if there is nothing to
 * save and the caller should exit itself.
 */
int enj_vmu_save_exit_after_flush(void);

ENJ_END_DECLS

#endif // ENJ_VMU_H
