#pragma once

#include "Scene.h"
#include "game/session/GameSession.h"
#include "game/tank/Tank.h"
#include "game/map/GameMap.h"
#include "game/entities/Base.h"
#include "game/entities/PowerUp.h"
#include "game/systems/ParticleSystem.h"
#include "ecs/EntityManager.h"
#include "ui/HUD.h"
#include "core/EventSystem.h"
#include "net/NetProtocol.h"
#include "net/NetworkOwner.h"
#include "scene/GameNetworkHost.h"
#include <vector>
#include <memory>

class NetController;
class SceneManager;

/// 游戏主场景：支持三种模式 (传统/攻防/各自为战)
class GameScene : public Scene, public IEventListener {
public:
    GameScene(SceneManager* manager, GameSession session);
    ~GameScene() override;

    void Enter() override;
    void Exit() override;
    void Update(float dt) override;
    void Render() override;

    void OnEvent(const Event& event) override;

    /// 设置网络管理器并接管所有权 (Host 模式)
    void SetNetworkManager(NetworkManager* net);

private:
    /// 网络 Host: 接收并应用客户端输入到 NetController
    void NetworkReceiveInput();
    void SpawnAllTanks();
    void SpawnBases();
    void SpawnPowerUp(float x, float y);
    void SetupPowerUpCollision(PowerUp* pu);
    void CheckGameOver();
    int GetAliveCount(int team) const;
    Tank* FindTankByEntity(Entity* e);

    /// 攻防战重生: 检查并处理守方重生
    void UpdateRespawns(float dt);

    SceneManager* manager_;
    GameSession session_;
    GameMode mode_;
    GameMap map_;
    EntityManager entityManager_;
    ParticleSystem particles_;
    HUD hud_;

    std::vector<std::unique_ptr<Tank>> tanks_;
    std::vector<Tank*> tankPtrs_;
    Base* bases_[2] = { nullptr, nullptr };
    int baseCount_ = 0;  // 实际基地数量 (0=FFA, 1=A/D, 2=Traditional)
    std::vector<std::pair<float,float>> pendingPowerUps_;

    bool gameOver_ = false;
    int winningTeam_ = -1;  // 获胜队伍 (FFA下为获胜玩家索引)
    float gameOverTimer_ = 0.f;

    // ---- 攻防战重生系统 ----
    float gameTimer_ = 0.f;  // 游戏计时器

    // ---- 网络 Host ----
    NetworkManagerPtr ownedNet_;
    NetworkManager* net_ = nullptr;
    GameNetworkHost netHost_;
    std::vector<NetController*> netControllers_;
};
