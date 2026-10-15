# Sound

[Documentation](README.md)

enDjinn plays short sound effects through the KOS sound-effect manager. Sounds
are converted at build time by `dcaconv` into `.dca` files; see
[Assets](assets.md#sound-effects). Declarations are in
[`enj_sound.h`](../include/enDjinn/enj_sound.h), and the
[sounds example](../examples/enj_sounds/code/enj_sounds.c) plays every format.

## Loading and playing

```c
sfxhnd_t hit = enj_sound_dca_load_file(ENJ_CBASEPATH "sfx/ADPCM/hit.dca");
if (hit != SFXHND_INVALID) {
  enj_sound_play(hit, 255, 128);
}
```

- `enj_sound_dca_load_file()` and `enj_sound_dca_load_blob()` validate the
  DCA header and load the samples into sound memory with
  `snd_sfx_load_raw_buf()`. They return `SFXHND_INVALID` on failure. A blob
  may be released after the call.
- `enj_sound_play(handle, volume, pan)` takes volume 0–255 and pan 0–255
  (128 is centre) and returns the channel, or -1 on error.
- `enj_sound_unload()` frees the sound.

`enj_state_startup()` initialises the KOS sound system; there is no separate
sound initialisation call. DCA files may be mono or stereo, and 4-bit ADPCM,
8-bit PCM or 16-bit PCM. The DCA format itself is implemented in
[`ext/dca_file.h`](../include/enDjinn/ext/dca_file.h).

## Host backends

pc-enDjinn and web-enDjinn play sounds through SDL audio, but load only
16-bit PCM. Loading an 8-bit PCM or ADPCM `.dca` returns `SFXHND_INVALID` on
those backends, so a portable project uses the `PCM/16` directory for sounds
it needs everywhere.

Music streaming and other uses of the AICA are outside enDjinn. For
sequenced music and DSP effects, see
[AICAflow](https://github.com/dfchil/AICAflow), whose interactive examples are
built with enDjinn.
