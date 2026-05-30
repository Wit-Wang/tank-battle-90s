#include "AudioManager.h"

AudioManager& AudioManager::Instance() {
    static AudioManager instance;
    return instance;
}

AudioManager::AudioManager() {
    InitAudioDevice();
}

AudioManager::~AudioManager() {
    UnloadAll();
    CloseAudioDevice();
}

void AudioManager::LoadSound(const std::string& name, const std::string& path) {
    if (sounds_.contains(name)) return;
    sounds_[name] = ::LoadSound(path.c_str());
}

void AudioManager::RegisterSound(const std::string& name, ::Sound sound) {
    if (sounds_.contains(name)) {
        ::UnloadSound(sounds_[name]);
    }
    sounds_[name] = sound;
}

void AudioManager::PlaySound(const std::string& name) {
    auto it = sounds_.find(name);
    if (it != sounds_.end()) {
        ::PlaySound(it->second);
    }
}

void AudioManager::UnloadAll() {
    for (auto& [name, snd] : sounds_) {
        ::UnloadSound(snd);
    }
    sounds_.clear();
}
