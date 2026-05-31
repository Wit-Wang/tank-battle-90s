#include "SoundGenerator.h"
#include "core/AudioManager.h"
#include "raylib.h"
#include <cmath>
#include <cstdlib>

static constexpr int SAMPLE_RATE = 48000;
static constexpr int CHANNELS = 2;

static void GenerateAndRegister(const std::string& name, float duration,
                                 float freqStart, float freqEnd, float volume = 0.5f) {
    int frameCount = static_cast<int>(SAMPLE_RATE * duration);
    int sampleCount = frameCount * CHANNELS;

    // 使用 16 位整数格式，兼容所有平台音频设备
    int16_t* data = new int16_t[sampleCount];
    for (int i = 0; i < frameCount; i++) {
        float t = static_cast<float>(i) / SAMPLE_RATE;
        float progress = t / duration;

        float freq = freqStart + (freqEnd - freqStart) * progress;
        float phase = 2.0f * 3.14159265f * freq * t;

        float envelope = (1.0f - progress);
        envelope *= envelope;

        float sample = sinf(phase) * envelope * volume;

        // 低频音效加噪声
        if (freqStart < 100.f) {
            float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.3f;
            sample += noise * envelope * volume;
        }

        if (sample > 1.f) sample = 1.f;
        if (sample < -1.f) sample = -1.f;

        // 转换为 16 位整数
        auto pcm = static_cast<int16_t>(sample * 32767.f);
        data[i * 2]     = pcm;
        data[i * 2 + 1] = pcm;
    }

    Wave wave = { 0 };
    wave.sampleRate = SAMPLE_RATE;
    wave.sampleSize = 16;
    wave.channels = CHANNELS;
    wave.frameCount = frameCount;
    wave.data = data;

    Sound sound = LoadSoundFromWave(wave);
    if (sound.frameCount > 0) {
        AudioManager::Instance().RegisterSound(name, sound);
    }

    delete[] data;
}

void SoundGenerator::GenerateAll() {
    GenerateAndRegister("shoot", 0.1f, 800.f, 400.f, 0.3f);
    GenerateAndRegister("explosion", 0.3f, 200.f, 50.f, 0.5f);
    GenerateAndRegister("pickup", 0.15f, 400.f, 800.f, 0.4f);
    GenerateAndRegister("gameover", 0.5f, 600.f, 100.f, 0.4f);
}
