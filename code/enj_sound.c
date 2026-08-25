#include <enDjinn/enj_defs.h>
#include <enDjinn/enj_sound.h>
#include <stdio.h>
#include <malloc.h>

#define DCAUDIO_IMPLEMENTATION
#include <enDjinn/ext/dca_file.h>

sfxhnd_t enj_sound_dca_load_file(const char* filename) {
  if (filename == NULL) {
    return SFXHND_INVALID;
  }
  sfxhnd_t handle = SFXHND_INVALID;
  uint8_t *buffer = NULL;
  FILE* sndfile = fopen(filename, "rb");
  if (sndfile) {
    long file_end = fseek(sndfile, 0, SEEK_END) == 0 ? ftell(sndfile) : -1;
    if (file_end > 0 && fseek(sndfile, 0, SEEK_SET) == 0) {
      size_t filesize = (size_t)file_end;
      buffer = memalign(32, filesize);
      if (buffer != NULL) {
        size_t amountread = fread(buffer, 1, filesize, sndfile);
        if (amountread == filesize) {
          handle = enj_sound_dca_load_blob(buffer);
        } else {
          ENJ_DEBUG_PRINT("enj_sound_dca_load_file: could not read entire file %s\n", filename);
        }
      } else {
        ENJ_DEBUG_PRINT("enj_sound_dca_load_file: allocation failed for %s\n", filename);
      }
    } else {
      ENJ_DEBUG_PRINT("enj_sound_dca_load_file: could not determine size of %s\n", filename);
    }
    fclose(sndfile);
  } else {
    ENJ_DEBUG_PRINT("enj_sound_dca_load_file: could not open file %s\n", filename);
  }
  if (buffer) {
    free(buffer);
  }
  
  return handle;
}

sfxhnd_t enj_sound_dca_load_blob(const uint8_t *dca_data) {
  if (dca_data == NULL) {
    return SFXHND_INVALID;
  }
  /* KOS's raw-buffer loader and the DCA accessor predate const-correctness;
     neither mutates the caller's blob. */
  fDcAudioHeader *data = (fDcAudioHeader *)(uintptr_t)dca_data;

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
