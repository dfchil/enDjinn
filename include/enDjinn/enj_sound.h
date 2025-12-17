#ifndef ENJ_SOUND_H
#define ENJ_SOUND_H

#include <dc/sound/sfxmgr.h>

sfxhnd_t enj_sound_load_wav_file(const char* filename);

sfxhnd_t enj_sound_load_wav_blob(uint8_t* data);

sfxhnd_t enj_sound_load_dca_file(const char* filename);

sfxhnd_t enj_sound_load_dca_blob(uint8_t* data);

void enj_sound_unload(sfxhnd_t handle);

int enj_sound_play(sfxhnd_t handle, uint8_t volume, uint8_t pan);


#endif // ENJ_SOUND_H