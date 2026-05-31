#pragma once

#include "GameMap.h"
#include <vector>
#include <string>

/// 地图管理器：扫描可用地图，提供选择切换
class MapManager {
public:
    MapManager() = default;

    /// 扫描目录下的地图文件 (所有子目录), 读取元数据
    void ScanMaps(const std::string& directory);

    /// 扫描指定模式的地图子目录
    void ScanMapsForMode(const std::string& baseDir, GameMode mode);

    /// 获取支持指定模式的地图索引列表
    std::vector<int> GetMapsForMode(GameMode mode) const;

    /// 设置当前索引
    void SetCurrentIndex(int idx) { if (idx >= 0 && idx < GetCount()) currentIndex_ = idx; }

    /// 切换地图
    void Next();
    void Previous();

    /// 重置到第一张地图
    void Reset() { currentIndex_ = 0; }

    /// 按索引获取地图信息
    const MapInfo& GetMapInfo(int index) const;

    /// 获取当前地图信息
    const MapInfo& GetCurrentMapInfo() const;

    /// 加载当前选中地图
    GameMap LoadCurrentMap() const;

    /// 获取地图总数
    int GetCount() const { return static_cast<int>(maps_.size()); }

    /// 获取当前索引
    int GetCurrentIndex() const { return currentIndex_; }

private:
    void ScanDirectory(const std::string& directory, GameMode mode);
    std::vector<MapInfo> maps_;
    int currentIndex_ = 0;
};
