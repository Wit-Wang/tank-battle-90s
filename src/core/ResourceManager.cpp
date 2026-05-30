#include "ResourceManager.h"

ResourceManager& ResourceManager::Instance() {
    static ResourceManager instance;
    return instance;
}

ResourceManager::~ResourceManager() {
    UnloadAll();
}

Texture2D& ResourceManager::LoadTexture(const std::string& name, const std::string& path) {
    if (!textures_.contains(name)) {
        textures_[name] = ::LoadTexture(path.c_str());
    }
    return textures_[name];
}

void ResourceManager::RegisterTexture(const std::string& name, Texture2D tex) {
    if (textures_.contains(name)) {
        ::UnloadTexture(textures_[name]);
    }
    textures_[name] = tex;
}

Texture2D& ResourceManager::GetTexture(const std::string& name) {
    return textures_.at(name);
}

bool ResourceManager::HasTexture(const std::string& name) const {
    return textures_.contains(name);
}

void ResourceManager::UnloadAll() {
    for (auto& [name, tex] : textures_) {
        ::UnloadTexture(tex);
    }
    textures_.clear();
}
