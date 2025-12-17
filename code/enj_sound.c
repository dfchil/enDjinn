#include <enDjinn/enj_defs.h>
#include <enDjinn/enj_sound.h>

#define DCAUDIO_IMPLEMENTATION
#include <enDjinn/ext/dca_file.h>

sfxhnd_t enj_sound_load_wav_file(const char* filename) {
  sfxhnd_t handle = snd_sfx_load(filename);
  if (handle == SFXHND_INVALID) {
    ENJ_DEBUG_PRINT("Failed to load WAV sound: %s\n", filename);
  }
  return handle;
}

sfxhnd_t enj_sound_load_wav_blob(uint8_t* data) {
  sfxhnd_t handle = snd_sfx_load_buf((char*)data);
  if (handle == SFXHND_INVALID) {
    ENJ_DEBUG_PRINT("Failed to load WAV sound from blob\n");
  }
  return handle;
}

sfxhnd_t enj_sound_load_dca_file(const char* filename) {
  // sfxhnd_t handle = snd_sfx_load(filename);
  // if (handle == SFXHND_INVALID) {
  //   ENJ_DEBUG_PRINT("Failed to load DCA sound: %s\n", filename);
  // }
  return SFXHND_INVALID;
}

sfxhnd_t enj_sound_load_dca_blob(uint8_t* data) {

  fDcAudioHeader *dca = (fDcAudioHeader*)data;
  printf("DCA fourcc: %.4s\n", dca->fourcc);

  if (fDaValidateHeader(dca)) {
    sfxhnd_t handle = snd_sfx_load_buf(data + sizeof(fDcAudioHeader));
    if (handle == SFXHND_INVALID) {
      ENJ_DEBUG_PRINT("Failed to load DCA sound from blob\n");
    }
    return handle;
  }
  

  return SFXHND_INVALID; 
}

void enj_sound_unload(sfxhnd_t handle) { snd_sfx_unload(handle); }

int enj_sound_play(sfxhnd_t handle, uint8_t volume, uint8_t pan) {
  if (handle == SFXHND_INVALID) {
    return -1;
  }
  return snd_sfx_play(handle, volume, pan);
}
