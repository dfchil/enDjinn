#include <enDjinn/enj_defs.h>
#include <enDjinn/enj_sound.h>
#include <stdio.h>
#include <malloc.h>

#define DCAUDIO_IMPLEMENTATION
#include <enDjinn/ext/dca_file.h>

sfxhnd_t enj_sound_dca_load_file(const char* filename) {
  sfxhnd_t handle = SFXHND_INVALID;
  uint8_t *buffer = NULL;
  FILE* sndfile = fopen(filename, "rb");
  if (sndfile) {
    fseek(sndfile, 0, SEEK_END);
    size_t filesize = ftell(sndfile);
    fseek(sndfile, 0, SEEK_SET);
    
    buffer = memalign(32, filesize);
    size_t amountread = fread(buffer, 1, filesize, sndfile);
    fclose(sndfile);
    if (amountread == filesize) {
      handle = enj_sound_dca_load_blob(buffer);
    } else {
      ENJ_DEBUG_PRINT("enj_sound_dca_load_file: could not read entire file %s\n", filename);
    }
  } else {
    ENJ_DEBUG_PRINT("enj_sound_dca_load_file: could not open file %s\n", filename);
  }
  if (buffer) {
    free(buffer);
  }
  
  return handle;
}

sfxhnd_t enj_sound_dca_load_blob(uint8_t* dca_data) {
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
