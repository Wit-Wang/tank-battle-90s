#pragma once

#include "net/NetworkManager.h"
#include "net/NetProtocol.h"
#include "net/NetController.h"
#include "net/NetController.h"
#include "scene/GameNetworkHost.h"
#include "game/session/GameSession.h"
#include "game/map/GameMap.h"
#include "game/map/MapManager.h"
#include "game/tank/Tank.h"
#include "game/entities/Base.h"
#include "game/entities/PowerUp.h"
#include "ecs/EntityManager.h"
#include "core/EventSystem.h"
#include <vector>
#include <memory>
#include <chrono>

/// Headless game server: accepts clients, manages lobby, runs authoritative game simulation
class GameServer : public IEventListener {
public:
    GameServer();
    ~GameServer() override;

    bool Start(uint16_t port = NET_DEFAULT_PORT);
    void Run();
    void Stop();

    /// 设置外部运行标志 (信号处理器设置, 用于 Ctrl+C)
    void SetRunningFlag(volatile bool* flag) { extRunning_ = flag; }

    void OnEvent(const Event& event) override;

private:
    // ---- State machine ----
    enum class State { LOBBY, GAME, GAME_OVER };
    State state_ = State::LOBBY;
    bool running_ = false;
    volatile bool* extRunning_ = nullptr;  // 外部信号标志 (SIGINT)

    /// 非阻塞检查 stdin 是否有 'q' 输入
    static bool CheckQuitKey();

    // ---- Network ----
    NetworkManager net_;
    void AcceptNewConnections();

    // ---- Lobby ----
    GameSession session_;
    MapManager mapManager_;
    int mapIndex_ = 0;
    float lobbyTimer_ = 0.f;
    float heartbeatTimer_ = 0.f;

    void LobbyUpdate(float dt);
    void HandleLobbyMessage(int clientSocket, const NetMessage& msg);
    void BroadcastLobbyState();
    void StartGame();
    bool AllClientsReady() const;

    // ---- Game simulation ----
    GameMap map_;
    EntityManager entityManager_;
    std::vector<std::unique_ptr<Tank>> tanks_;
    std::vector<Tank*> tankPtrs_;
    Base* bases_[2] = { nullptr, nullptr };
    int baseCount_ = 0;
    std::vector<std::pair<float,float>> pendingPowerUps_;
    GameNetworkHost netHost_;
    std::vector<NetController*> netControllers_;  // non-owning, owned by Tank

    float gameTimer_ = 0.f;
    bool gameOver_ = false;
    int winningTeam_ = -1;
    float gameOverTimer_ = 0.f;
    float stateTimer_ = 0.f;

    void GameUpdate(float dt);
    void SpawnAllTanks();
    void SpawnBases();
    void SpawnPowerUp(float x, float y);
    void SetupPowerUpCollision(PowerUp* pu);
    void CheckGameOver();
    int GetAliveCount(int team) const;
    void NetworkReceiveInput();
    void UpdateRespawns(float dt);
    Tank* FindTankByEntity(Entity* e);
    void CleanupGame();

    // ---- Timing ----
    static constexpr float TICK_RATE = 1.f / 60.f;
    static constexpr float GAME_OVER_DURATION = 5.f;
};
