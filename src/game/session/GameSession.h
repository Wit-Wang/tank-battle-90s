#pragma once

#include "PlayerSlot.h"
#include "core/Types.h"
#include <array>

class MapManager;

/// 游戏会话：管理 4 个玩家槽位的配置与状态
class GameSession {
public:
    GameSession();

    /// 设置游戏模式 (影响槽位预设、重生规则等)
    void SetGameMode(GameMode mode);

    /// 获取游戏模式
    GameMode GetGameMode() const { return mode_; }

    /// 应用地图选择结果
    void ApplyMapSelection(const MapManager& mm);

    /// 随机分配所有 AI 槽位的坦克类型
    void RandomizeAITypes();

    /// 根据人类队伍分配，自动平衡 AI 队伍 (每队2人)
    void BalanceAITeams();

    /// 获取槽位
    PlayerSlot& GetSlot(int index);
    const PlayerSlot& GetSlot(int index) const;

    /// 获取地图文件路径
    const std::string& GetMapPath() const { return mapPath_; }
    void SetMapPath(const std::string& path) { mapPath_ = path; }

    static constexpr int SLOT_COUNT = 4;

private:
    GameMode mode_ = GameMode::TRADITIONAL;
    std::array<PlayerSlot, SLOT_COUNT> slots_;
    std::string mapPath_;
};
