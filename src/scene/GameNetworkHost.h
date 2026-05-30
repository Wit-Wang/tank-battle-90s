#pragma once

#include "net/NetProtocol.h"
#include "game/map/TileType.h"
#include <vector>
#include <cstdint>

class NetworkManager;
class Tank;
class EntityManager;
class GameMap;

/// GameScene 的网络 Host 职责提取
/// 负责：状态广播、客户端输入接收、地图变化同步
class GameNetworkHost {
public:
    GameNetworkHost() = default;
    explicit GameNetworkHost(NetworkManager* net) : net_(net) {}

    /// 设置网络管理器
    void SetNetwork(NetworkManager* net) { net_ = net; }

    /// 初始化网络 (绑定 UDP、保存地图快照)
    void Init(GameMap& map);

    /// 广播当前游戏状态 (4 玩家 + 所有子弹)，20Hz
    void BroadcastState(const std::vector<std::unique_ptr<Tank>>& tanks,
                        EntityManager& em);

    /// 检测并广播地图瓦片变化 (增量同步)
    void CheckMapChanges(GameMap& map);

    /// 广播游戏结束消息
    void BroadcastGameOver(int winningTeam);

    /// 是否处于联网模式
    bool IsNetworked() const { return net_ != nullptr; }

private:
    NetworkManager* net_ = nullptr;
    float broadcastTimer_ = 0.f;
    uint16_t stateSeq_ = 0;
    std::vector<std::vector<TileType>> prevMapTiles_;  // 地图快照 (变化检测)
};
