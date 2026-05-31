#pragma once

#ifdef TANKGAME_SERVER
#include "utils/raylib_stubs.h"
#else
#include "raylib.h"
#endif
#include <unordered_map>
#include <string>

/// 音频管理器 (单例)：缓存并播放音效与音乐
class AudioManager {
public:
    static AudioManager& Instance();

    void LoadSound(const std::string& name, const std::string& path);
    void RegisterSound(const std::string& name, ::Sound sound);
    void PlaySound(const std::string& name);
    void UnloadAll();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

private:
    AudioManager();
    ~AudioManager();

    std::unordered_map<std::string, ::Sound> sounds_;
};
