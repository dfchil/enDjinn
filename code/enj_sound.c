#include <enDjinn/enj_defs.h>
#include <enDjinn/enj_sound.h>
#include <stdio.h>

#define DCAUDIO_IMPLEMENTATION
#include <enDjinn/ext/dca_file.h>

sfxhnd_t enj_sound_load_dca_file(const char* filename) {
  FILE* f = fopen(filename, "rb");
  if (f) {
    fseek(f, 0, SEEK_END);
    size_t filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t data[filesize];
    size_t amountread = fread(data, 1, filesize, f);
    fclose(f);
    if (amountread == filesize) {
      return enj_sound_load_dca_blob(data);
    } else {
      ENJ_DEBUG_PRINT("enj_sound_load_dca_file: could not read entire file %s\n", filename);
    }
  } else {
    ENJ_DEBUG_PRINT("enj_sound_load_dca_file: could not open file %s\n", filename);
  }
  return SFXHND_INVALID;
}

sfxhnd_t enj_sound_load_dca_blob(uint8_t* dca_data) {
  fDcAudioHeader* data = (fDcAudioHeader*)dca_data;

  if (fDaValidateHeader(data)) {
    uint8_t bitsize = (uint8_t[]){16, 8, 4}[fDaGetSampleFormat(data)];

    int channels = fDaGetChannelCount(data);
    if (channels == 2) {
      return snd_sfx_load_raw_buf(fDaGetChannelSamples(data, 0), fDaCalcChannelSizeBytes(data),
                                  fDaCalcSampleRateHz(data), bitsize, 2);
    }

    return snd_sfx_load_raw_buf(fDaGetChannelSamples(data, 0), fDaCalcChannelSizeBytes(data),
                                fDaCalcSampleRateHz(data), bitsize, 1);
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
