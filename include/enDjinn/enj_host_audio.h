#ifndef ENJ_HOST_AUDIO_SOURCE_H
#define ENJ_HOST_AUDIO_SOURCE_H

#include <stdint.h>

#include <enDjinn/enj_api.h>

ENJ_BEGIN_DECLS

/**
 * Host targets only (pc-enDjinn and web-enDjinn): one more audio source mixed
 * into the SDL output, for a library that synthesises its own sound, such as
 * AICAflow's host backend. On the Dreamcast the AICA is the output and there
 * is nothing to attach.
 *
 * The source fills interleaved 16-bit stereo at 44100 Hz. It runs on SDL's
 * audio thread on a PC and on the main thread between frames in a browser.
 */
typedef void (*enj_host_audio_source_fn)(int16_t *stereo, uint32_t frames, void *user);

/**
 * Sets the source; NULL removes it. Opens the audio device if it is not open
 * yet, as for a program that sets enj_state flags.no_snd_init. Sound effects
 * played with snd_sfx_play() are mixed on top. Returns 0, or -1 when no audio
 * device could be opened.
 */
int enj_host_audio_source_set(enj_host_audio_source_fn source, void *user);

ENJ_END_DECLS

#endif
