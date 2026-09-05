#include "host_audio.h"

#include <kos.h>

#include <SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

struct Sound {
    std::vector<int16_t> samples;
    uint32_t rate;
};

struct Voice {
    int sound = SFXHND_INVALID;
    double frame = 0.0;
    uint8_t volume = 0;
    uint8_t pan = 0;
};

SDL_AudioDeviceID g_audio_device = 0;
SDL_AudioSpec g_audio_spec{};
std::vector<Sound> g_sounds;
std::vector<Voice> g_voices;

int16_t clamp_sample(int value)
{
    return static_cast<int16_t>(std::clamp(value, -32768, 32767));
}

void audio_callback(void *, Uint8 *stream, int bytes)
{
    auto *output = reinterpret_cast<int16_t *>(stream);
    const int frames = bytes / static_cast<int>(sizeof(*output) * 2u);
    std::memset(stream, 0, static_cast<size_t>(bytes));
    for (Voice &voice : g_voices) {
        if (voice.sound < 0 || static_cast<size_t>(voice.sound) >= g_sounds.size()) {
            continue;
        }
        const Sound &sound = g_sounds[voice.sound];
        const size_t sound_frames = sound.samples.size() / 2u;
        const double step = static_cast<double>(sound.rate) / g_audio_spec.freq;
        for (int frame = 0;
             frame < frames && static_cast<size_t>(voice.frame) < sound_frames;
             frame++, voice.frame += step) {
            const size_t source = static_cast<size_t>(voice.frame) * 2u;
            const int left_gain = voice.volume * (255 - voice.pan);
            const int right_gain = voice.volume * voice.pan;
            output[frame * 2] = clamp_sample(
                output[frame * 2] +
                sound.samples[source] * left_gain / 65025);
            output[frame * 2 + 1] = clamp_sample(
                output[frame * 2 + 1] +
                sound.samples[source + 1u] * right_gain / 65025);
        }
    }
    g_voices.erase(
        std::remove_if(g_voices.begin(), g_voices.end(), [](const Voice &voice) {
            return voice.sound < 0 ||
                static_cast<size_t>(voice.sound) >= g_sounds.size() ||
                static_cast<size_t>(voice.frame) >=
                    g_sounds[voice.sound].samples.size() / 2u;
        }),
        g_voices.end());
}

}  // namespace

namespace enj_host {

void audio_shutdown()
{
    if (g_audio_device != 0) {
        SDL_CloseAudioDevice(g_audio_device);
        g_audio_device = 0;
    }
    g_voices.clear();
    g_sounds.clear();
}

}  // namespace enj_host

extern "C" {

void snd_init(void)
{
    if (g_audio_device != 0) {
        return;
    }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::fprintf(stderr, "host-enDjinn: SDL audio initialization failed: %s\n",
                     SDL_GetError());
        return;
    }
    SDL_AudioSpec wanted{};
    wanted.freq = 44100;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 2;
    wanted.samples = 1024;
    wanted.callback = audio_callback;
    g_audio_device = SDL_OpenAudioDevice(nullptr, 0, &wanted, &g_audio_spec, 0);
    if (g_audio_device == 0) {
        std::fprintf(stderr, "host-enDjinn: SDL audio open failed: %s\n",
                     SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(g_audio_device, 0);
}

sfxhnd_t snd_sfx_load_raw_buf(void *samples, size_t size, uint32_t sample_rate,
                              uint8_t bits, uint8_t channels)
{
    if (samples == nullptr || bits != 16u ||
        (channels != 1u && channels != 2u) || size < 2u) {
        return SFXHND_INVALID;
    }
    const auto *input = static_cast<const int16_t *>(samples);
    const size_t frames = size / sizeof(*input);
    Sound sound{{}, sample_rate};
    sound.samples.resize(frames * 2u);
    for (size_t frame = 0; frame < frames; frame++) {
        sound.samples[frame * 2u] = input[frame];
        sound.samples[frame * 2u + 1u] =
            channels == 2u ? input[frames + frame] : input[frame];
    }
    if (g_audio_device != 0) {
        SDL_LockAudioDevice(g_audio_device);
    }
    g_sounds.push_back(std::move(sound));
    const sfxhnd_t handle = static_cast<sfxhnd_t>(g_sounds.size() - 1u);
    if (g_audio_device != 0) {
        SDL_UnlockAudioDevice(g_audio_device);
    }
    return handle;
}

void snd_sfx_unload(sfxhnd_t handle)
{
    if (handle < 0 || static_cast<size_t>(handle) >= g_sounds.size()) {
        return;
    }
    if (g_audio_device != 0) {
        SDL_LockAudioDevice(g_audio_device);
    }
    g_sounds[handle].samples.clear();
    if (g_audio_device != 0) {
        SDL_UnlockAudioDevice(g_audio_device);
    }
}

int snd_sfx_play(sfxhnd_t handle, uint8_t volume, uint8_t pan)
{
    if (g_audio_device == 0 || handle < 0 ||
        static_cast<size_t>(handle) >= g_sounds.size()) {
        return -1;
    }
    SDL_LockAudioDevice(g_audio_device);
    g_voices.push_back({handle, 0.0, volume, pan});
    SDL_UnlockAudioDevice(g_audio_device);
    return 0;
}

}  // extern "C"
