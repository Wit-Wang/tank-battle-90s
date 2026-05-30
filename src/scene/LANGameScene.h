#pragma once

#include "Scene.h"
#include "game/session/GameSession.h"
#include "game/map/GameMap.h"
#include "game/map/TileType.h"
#include "net/NetProtocol.h"
#include "net/NetworkOwner.h"
#include "core/Timer.h"
#include <vector>
#include <string>

class SceneManager;

/// 客户端游戏场景: 接收 Host 状态并渲染
///
/// 所有权: 接管 NetworkManager (通过 raw pointer, 由 LANLobbyScene::ReleaseNetwork() 转移)
class LANGameScene : public Scene {
public:
    LANGameScene(SceneManager* manager, NetworkManager* net, GameSession session, int mySlot = -1);
    ~LANGameScene() override;

    void Enter() override;
    void Exit() override;
    void Update(float dt) override;
    void Render() override;

private:
    void ProcessMessages();
    /// 解析 GameState 消息 (TCP/UDP 共用)
    void ParseGameState(const NetMessage& msg);
    void SendInput();
    void RenderPlayers();
    void RenderBullets();
    void RenderHUD();

    // 客户端玩家数据
    struct RemotePlayer {
        float x = 0, y = 0;
        float rotation = 0;
        uint8_t hp = 0, maxHp = 1;
        bool alive = false;
        bool invincible = false;
        TankType type = TankType::MEDIUM;
        int team = 0;
    };

    struct RemoteBullet {
        uint8_t id = 0;
        float x = 0, y = 0;
        uint8_t ownerTeam = 0;
        bool active = false;
    };

    SceneManager* manager_;
    NetworkManagerPtr ownedNet_;                // 接管所有权
    NetworkManager* net_;                       // 便利指针
    GameSession session_;
    int mySlot_ = -1;                          // 客户端自己的槽位
    GameMode mode_ = GameMode::TRADITIONAL;

    GameMap map_;
    std::vector<RemotePlayer> players_;
    std::vector<RemoteBullet> bullets_;

    float inputTimer_ = 0.f;
    float disconnectTimer_ = 0.f;
    bool gameOver_ = false;
    int winningTeam_ = -1;
    float gameOverTimer_ = 0.f;
    bool gameStarted_ = false;

    // 服务器状态序列号 (用于检测丢包)
    uint16_t lastStateSeq_ = 0;
};
