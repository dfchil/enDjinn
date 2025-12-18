#ifndef ENJ_SOUND_H
#define ENJ_SOUND_H

#include <dc/sound/sfxmgr.h>

/**
 * Load a DCA sound effect from a memory blob
 * @param data Pointer to the DCA data in memory
 * @return The sound effect handle, or SFXHND_INVALID on error
 */
sfxhnd_t enj_sound_load_dca_blob(uint8_t* data);

/**
 * Load a DCA sound effect from a file
 * @param filename Path to the DCA file to load
 * @return The sound effect handle, or SFXHND_INVALID on error
 */
sfxhnd_t enj_sound_load_dca_file(const char* filename);

/**
 * Unload a sound effect previously loaded with enj_sound_load_dca_file or enj_sound_load_dca_blob
 * @param handle The sound effect handle to unload
 * @return void
 */
void enj_sound_unload(sfxhnd_t handle);

/**
 * Play a sound effect previously loaded with enj_sound_load_dca_file or enj_sound_load_dca_blob
 * @param handle The sound effect handle to play
 * @param volume Volume to play the sound effect at (0-255)
 * @param pan Pan to play the sound effect at (0-255, 128 is center)
 * @return The channel the sound is playing on, or -1 on error
 */
int enj_sound_play(sfxhnd_t handle, uint8_t volume, uint8_t pan);


#endif // ENJ_SOUND_H