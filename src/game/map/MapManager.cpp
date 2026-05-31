#include "MapManager.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

/// 从地图文件读取 #modes: 元数据头, 推断支持的模式
static void ReadMapMetadata(MapInfo& info) {
    std::ifstream file(info.filePath);
    if (!file.is_open()) return;

    std::string line;
    int baseCount = 0;
    bool foundHeader = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // 解析元数据头
        if (line[0] == '#') {
            const std::string tag = "#modes:";
            if (line.rfind(tag, 0) == 0) {
                std::string modes = line.substr(tag.size());
                std::istringstream ms(modes);
                std::string modeName;
                while (std::getline(ms, modeName, ',')) {
                    while (!modeName.empty() && modeName.front() == ' ') modeName.erase(0, 1);
                    while (!modeName.empty() && modeName.back() == ' ') modeName.pop_back();
                    if (modeName == "traditional")      info.supportedModes.push_back(GameMode::TRADITIONAL);
                    else if (modeName == "attack_defend") info.supportedModes.push_back(GameMode::ATTACK_DEFEND);
                    else if (modeName == "ffa")         info.supportedModes.push_back(GameMode::FREE_FOR_ALL);
                }
                foundHeader = true;
            }
            continue;
        }

        // 统计基地瓦片数量 (用于无元数据时推断)
        std::istringstream iss(line);
        int val;
        while (iss >> val) {
            if (val == 5) baseCount++;  // TileType::BASE = 5
        }
    }

    // 无元数据头时根据基地数量推断
    if (!foundHeader) {
        if (baseCount >= 2) {
            info.supportedModes = { GameMode::TRADITIONAL, GameMode::ATTACK_DEFEND, GameMode::FREE_FOR_ALL };
        } else if (baseCount == 1) {
            info.supportedModes = { GameMode::ATTACK_DEFEND, GameMode::FREE_FOR_ALL };
        } else {
            info.supportedModes = { GameMode::FREE_FOR_ALL };
        }
    }

    // 设置主模式为第一个支持的模式
    if (!info.supportedModes.empty()) {
        info.mode = info.supportedModes[0];
    }
}

void MapManager::ScanDirectory(const std::string& directory, GameMode mode) {
    if (!fs::exists(directory)) return;

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            if (ext == ".txt" || ext == ".map") {
                MapInfo info;
                info.name = entry.path().stem().string();
                info.filePath = entry.path().string();
                info.difficulty = 1;
                info.mode = mode;
                ReadMapMetadata(info);
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

std::vector<int> MapManager::GetMapsForMode(GameMode mode) const {
    std::vector<int> result;
    for (int i = 0; i < static_cast<int>(maps_.size()); i++) {
        for (auto m : maps_[i].supportedModes) {
            if (m == mode) {
                result.push_back(i);
                break;
            }
        }
    }
    return result;
}
