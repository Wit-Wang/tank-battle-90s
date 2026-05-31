#pragma once

#ifdef TANKGAME_SERVER
#include "utils/raylib_stubs.h"
#else
#include "raylib.h"
#endif
#include <unordered_map>
#include <string>

/// 资源管理器 (单例)：缓存纹理，避免重复加载
class ResourceManager {
public:
    static ResourceManager& Instance();

    Texture2D& LoadTexture(const std::string& name, const std::string& path);
    void RegisterTexture(const std::string& name, Texture2D tex);
    Texture2D& GetTexture(const std::string& name);
    bool HasTexture(const std::string& name) const;
    void UnloadAll();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

private:
    ResourceManager() = default;
    ~ResourceManager();

    std::unordered_map<std::string, ::Texture2D> textures_;
};
