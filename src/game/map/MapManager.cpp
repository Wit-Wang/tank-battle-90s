#include "MapManager.h"
#include <filesystem>

namespace fs = std::filesystem;

void MapManager::ScanDirectory(const std::string& directory, GameMode mode) {
    if (!fs::exists(directory)) return;

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            if (ext == ".txt" || ext == ".map") {
                MapInfo info;
                info.name = entry.path().stem().string();
                info.filePath = entry.path().string();
                info.difficulty = 1;  // 默认难度
                info.mode = mode;
                maps_.push_back(info);
            }
        }
    }
}

void MapManager::ScanMaps(const std::string& directory) {
    maps_.clear();

    // 扫描各子目录
    ScanDirectory(directory + "/traditional", GameMode::TRADITIONAL);
    ScanDirectory(directory + "/attack_defend", GameMode::ATTACK_DEFEND);
    ScanDirectory(directory + "/ffa", GameMode::FREE_FOR_ALL);

    // 兼容: 也扫描根目录 (旧地图文件)
    if (fs::exists(directory)) {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                if (ext == ".txt" || ext == ".map") {
                    MapInfo info;
                    info.name = entry.path().stem().string();
                    info.filePath = entry.path().string();
                    info.difficulty = 1;
                    info.mode = GameMode::TRADITIONAL;  // 旧地图默认传统
                    maps_.push_back(info);
                }
            }
        }
    }

    // 按名称排序
    std::sort(maps_.begin(), maps_.end(),
        [](const MapInfo& a, const MapInfo& b) { return a.name < b.name; });

    // 如果没有地图文件，添加默认地图
    if (maps_.empty()) {
        MapInfo defaultMap;
        defaultMap.name = "Classic";
        defaultMap.filePath = "assets/maps/classic.txt";
        defaultMap.difficulty = 1;
        defaultMap.mode = GameMode::TRADITIONAL;
        maps_.push_back(defaultMap);
    }
}

void MapManager::ScanMapsForMode(const std::string& baseDir, GameMode mode) {
    maps_.clear();

    const char* subDir = "";
    switch (mode) {
        case GameMode::TRADITIONAL:   subDir = "/traditional";   break;
        case GameMode::ATTACK_DEFEND: subDir = "/attack_defend"; break;
        case GameMode::FREE_FOR_ALL:  subDir = "/ffa";           break;
    }

    ScanDirectory(baseDir + subDir, mode);

    // 按名称排序
    std::sort(maps_.begin(), maps_.end(),
        [](const MapInfo& a, const MapInfo& b) { return a.name < b.name; });

    // Fallback: 如果该模式没有地图，扫描根目录
    if (maps_.empty()) {
        ScanMaps(baseDir);
    }
}

void MapManager::Next() {
    if (!maps_.empty()) {
        currentIndex_ = (currentIndex_ + 1) % static_cast<int>(maps_.size());
    }
}

void MapManager::Previous() {
    if (!maps_.empty()) {
        currentIndex_ = (currentIndex_ + static_cast<int>(maps_.size()) - 1) %
                        static_cast<int>(maps_.size());
    }
}

const MapInfo& MapManager::GetCurrentMapInfo() const {
    return maps_[currentIndex_];
}

const MapInfo& MapManager::GetMapInfo(int index) const {
    return maps_[index];
}

GameMap MapManager::LoadCurrentMap() const {
    GameMap map;
    if (!maps_.empty()) {
        map.LoadFromFile(maps_[currentIndex_].filePath);
    }
    return map;
}
