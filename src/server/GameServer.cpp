// winsock2.h 必须在 windows.h (raylib) 之前包含
#include "net/NetworkManager.h"
#include "net/NetController.h"

#include "GameServer.h"
#include "game/entities/Bullet.h"
#include "game/tank/TankFactory.h"
#include "game/tank/controller/AIController.h"
#include "game/entities/Bullet.h"
#include "game/systems/CollisionSystem.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/HealthComponent.h"
#include "core/Types.h"
#include "utils/Random.h"
#include <cstdio>
#include <thread>
#include <chrono>
#include <algorithm>

#ifdef _WIN32
#include <conio.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif

GameServer::GameServer() = default;
GameServer::~GameServer() {
    Stop();
}

bool GameServer::CheckQuitKey() {
#ifdef _WIN32
    if (_kbhit()) {
        int ch = _getch();
        return ch == 'q' || ch == 'Q';
    }
    return false;
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = { 0, 0 };
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
        char buf[1];
        if (read(STDIN_FILENO, buf, 1) > 0) {
            return buf[0] == 'q' || buf[0] == 'Q';
        }
    }
    return false;
#endif
}

bool GameServer::Start(uint16_t port) {
    if (!net_.Listen(port)) {
        printf("[SERVER] Failed to listen on port %d\n", port);
        return false;
    }

    // Scan ALL maps with metadata
    mapManager_.ScanMaps("assets/maps");
    session_.SetGameMode(GameMode::TRADITIONAL);
    // Server: all slots start as AI (human slots assigned on connect)
    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        session_.GetSlot(i).isHuman = false;
    }
    RefreshModeMaps();

    printf("[SERVER] Listening on port %d\n", port);
    printf("[SERVER] Loaded %d maps (%d for current mode)\n",
           mapManager_.GetCount(), static_cast<int>(modeMapIndices_.size()));
    printf("[SERVER] Controls: Q/E=Mode  A/D=Map  Ctrl+C=Quit\n");
    printf("[SERVER] Waiting for players to connect and ready up...\n");

    running_ = true;
    state_ = State::LOBBY;
    nextSlot_ = 0;
    return true;
}

void GameServer::Stop() {
    running_ = false;
    CleanupGame();
    net_.DisconnectAllClients();
    net_.Close();
    net_.CloseUDP();
}

void GameServer::Run() {
    auto lastTime = std::chrono::steady_clock::now();

    while (running_) {
        // 检查外部退出信号 (Ctrl+C)
        if (extRunning_ && !*extRunning_) {
            printf("[SERVER] Signal received, shutting down...\n");
            break;
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        AcceptNewConnections();

        switch (state_) {
            case State::LOBBY:
                LobbyUpdate(dt);
                break;
            case State::GAME:
                GameUpdate(dt);
                break;
            case State::GAME_OVER:
                gameOverTimer_ += dt;
                if (gameOverTimer_ < 0.1f) {
                    netHost_.BroadcastGameOver(winningTeam_);
                }
                if (gameOverTimer_ >= GAME_OVER_DURATION) {
                    // Send ReturnToLobby to all clients
                    NetMessage rtl(NetMessageType::ReturnToLobby);
                    net_.BroadcastTCP(rtl);

                    CleanupGame();
                    state_ = State::LOBBY;
                    printf("[SERVER] Returned to lobby. Waiting for players...\n");
                }
                break;
        }

        // Fixed timestep sleep
        auto elapsed = std::chrono::steady_clock::now() - now;
        auto target = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<float>(TICK_RATE));
        if (elapsed < target) {
            std::this_thread::sleep_for(target - elapsed);
        }
    }
}

// ============================================================
//  Connection management
// ============================================================

void GameServer::AcceptNewConnections() {
    int newSock = net_.AcceptClient();
    if (newSock < 0) return;

    // Find an available slot (server has no host reservation — slot 0 is available)
    int slot = -1;
    auto& clients = net_.GetClients();
    clients.back().slotIndex = -1;  // clear pre-assigned value from AcceptClient()
    bool used[4] = {};
    for (auto& c : clients) {
        if (c.slotIndex >= 0 && c.slotIndex < 4)
            used[c.slotIndex] = true;
    }
    for (int i = 0; i < 4; i++) {
        if (!used[i]) { slot = i; break; }
    }

    if (slot < 0) {
        NetMessage kick(NetMessageType::Kick);
        kick.WriteString("Server full");
        net_.SendTo(newSock, kick);
        net_.DisconnectClient(newSock);
        printf("[SERVER] Rejected connection: server full\n");
        return;
    }

    // Assign slot and team by join order
    clients.back().slotIndex = slot;
    clients.back().lastPing = lobbyTimer_;
    clients.back().ready = false;
    session_.GetSlot(slot).isHuman = true;
    AssignTeamForSlot(slot);

    NetMessage accepted(NetMessageType::JoinAccepted);
    accepted.WritePayload(static_cast<uint8_t>(slot));
    net_.SendTo(newSock, accepted);

    NetMessage joined(NetMessageType::PlayerJoined);
    joined.WritePayload(static_cast<uint8_t>(slot));
    joined.WriteString("Player");
    net_.BroadcastTCP(joined);

    BroadcastLobbyState();

    printf("[SERVER] Player joined as P%d (team %d)\n",
           slot + 1, session_.GetSlot(slot).team);
}

// ============================================================
//  Lobby
// ============================================================

void GameServer::LobbyUpdate(float dt) {
    lobbyTimer_ += dt;

    // Process server stdin input (Q/E=mode, A/D=map)
    HandleServerInput();

    // Heartbeat: send periodic pings, detect disconnected clients
    heartbeatTimer_ += dt;
    if (heartbeatTimer_ >= NET_PING_INTERVAL) {
        heartbeatTimer_ = 0.f;
        NetMessage ping(NetMessageType::Ping);
        net_.BroadcastTCP(ping);

        // Check for timed-out clients
        auto& clients = net_.GetClients();
        for (int i = static_cast<int>(clients.size()) - 1; i >= 0; i--) {
            if (clients[i].lastPing > 0.f &&
                lobbyTimer_ - clients[i].lastPing > NET_TIMEOUT_SECONDS) {
                printf("[SERVER] Player P%d timed out\n", clients[i].slotIndex + 1);
                int slot = clients[i].slotIndex;
                session_.GetSlot(slot).isHuman = false;
                net_.DisconnectClient(clients[i].socket);
                BroadcastLobbyState();
            }
        }
    }

    // Process messages from all clients
    for (auto& client : net_.GetClients()) {
        NetMessage msg;
        while (net_.ReceiveFromClient(client.socket, msg)) {
            HandleLobbyMessage(client.socket, msg);
        }
    }

    // Start game when all connected players are ready (at least 1 player required)
    if (net_.ClientCount() > 0 && AllClientsReady()) {
        printf("[SERVER] All players ready, starting game\n");
        StartGame();
    }
}

bool GameServer::AllClientsReady() const {
    for (const auto& client : net_.GetClients()) {
        if (!client.ready) return false;
    }
    return true;
}

void GameServer::HandleLobbyMessage(int clientSocket, const NetMessage& msg) {
    switch (msg.header.type) {
        case NetMessageType::SetTankType: {
            uint8_t tankType = msg.ReadPayload<uint8_t>();
            int ci = net_.FindClientBySocket(clientSocket);
            if (ci >= 0) {
                int slot = net_.GetClients()[ci].slotIndex;
                if (slot >= 0 && slot < 4) {
                    session_.GetSlot(slot).tankType = static_cast<TankType>(tankType);
                }
            }
            BroadcastLobbyState();
            break;
        }
        case NetMessageType::ClientReady: {
            int ci = net_.FindClientBySocket(clientSocket);
            if (ci >= 0) {
                net_.GetClients()[ci].ready = !net_.GetClients()[ci].ready;
                printf("[SERVER] Player P%d ready=%d\n",
                       net_.GetClients()[ci].slotIndex + 1,
                       net_.GetClients()[ci].ready ? 1 : 0);
            }
            BroadcastLobbyState();
            break;
        }
        case NetMessageType::Ping: {
            NetMessage pong(NetMessageType::Pong);
            net_.SendTo(clientSocket, pong);
            break;
        }
        case NetMessageType::Pong: {
            // Client replied to our ping, update lastPing
            int ci = net_.FindClientBySocket(clientSocket);
            if (ci >= 0) {
                net_.GetClients()[ci].lastPing = lobbyTimer_;
            }
            break;
        }
        default:
            break;
    }
}

// ============================================================
//  Server input & mode/map management
// ============================================================

void GameServer::HandleServerInput() {
#ifdef _WIN32
    if (!_kbhit()) return;
    int ch = _getch();
    // 处理方向键前缀
    if (ch == 0 || ch == 0xE0) {
        ch = _getch();
        // 方向键暂不处理
        return;
    }
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = { 0, 0 };
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0) return;
    char buf[1];
    if (read(STDIN_FILENO, buf, 1) <= 0) return;
    int ch = buf[0];
#endif

    bool changed = false;

    if (ch == 'q' || ch == 'Q') {
        // Q: 切换到上一个模式
        int m = static_cast<int>(session_.GetGameMode());
        m = (m + 2) % 3;  // 0→2→1→0
        session_.SetGameMode(static_cast<GameMode>(m));
        // 保留已连接玩家的 human 状态
        for (auto& c : net_.GetClients()) {
            if (c.slotIndex >= 0 && c.slotIndex < 4)
                session_.GetSlot(c.slotIndex).isHuman = true;
        }
        RefreshModeMaps();
        changed = true;
        const char* modeNames[] = { "Traditional", "Attack/Defend", "Free for All" };
        printf("[SERVER] Mode: %s (%d maps)\n",
               modeNames[m], static_cast<int>(modeMapIndices_.size()));
    } else if (ch == 'e' || ch == 'E') {
        // E: 切换到下一个模式
        int m = static_cast<int>(session_.GetGameMode());
        m = (m + 1) % 3;
        session_.SetGameMode(static_cast<GameMode>(m));
        // 保留已连接玩家的 human 状态
        for (auto& c : net_.GetClients()) {
            if (c.slotIndex >= 0 && c.slotIndex < 4)
                session_.GetSlot(c.slotIndex).isHuman = true;
        }
        RefreshModeMaps();
        changed = true;
        const char* modeNames[] = { "Traditional", "Attack/Defend", "Free for All" };
        printf("[SERVER] Mode: %s (%d maps)\n",
               modeNames[m], static_cast<int>(modeMapIndices_.size()));
    } else if (ch == 'a' || ch == 'A') {
        // A: 上一张地图
        if (!modeMapIndices_.empty()) {
            modeMapIdx_ = (modeMapIdx_ + static_cast<int>(modeMapIndices_.size()) - 1) %
                          static_cast<int>(modeMapIndices_.size());
            mapIndex_ = modeMapIndices_[modeMapIdx_];
            changed = true;
            printf("[SERVER] Map: %s\n",
                   mapManager_.GetMapInfo(mapIndex_).name.c_str());
        }
    } else if (ch == 'd' || ch == 'D') {
        // D: 下一张地图
        if (!modeMapIndices_.empty()) {
            modeMapIdx_ = (modeMapIdx_ + 1) % static_cast<int>(modeMapIndices_.size());
            mapIndex_ = modeMapIndices_[modeMapIdx_];
            changed = true;
            printf("[SERVER] Map: %s\n",
                   mapManager_.GetMapInfo(mapIndex_).name.c_str());
        }
    }

    if (changed) {
        // 模式切换时重新分配所有队伍
        GameMode mode = session_.GetGameMode();
        if (mode == GameMode::FREE_FOR_ALL) {
            // FFA: 每人一队
            for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
                session_.GetSlot(i).team = i;
            }
        } else {
            // 传统/攻防: 按次序分队
            nextSlot_ = 0;
            for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
                if (session_.GetSlot(i).isHuman) {
                    AssignTeamForSlot(i);
                }
            }
        }
        BroadcastLobbyState();
    }
}

void GameServer::RefreshModeMaps() {
    GameMode mode = session_.GetGameMode();
    modeMapIndices_ = mapManager_.GetMapsForMode(mode);
    modeMapIdx_ = 0;

    if (modeMapIndices_.empty()) {
        // 没有该模式的地图, 添加默认
        printf("[SERVER] Warning: no maps for current mode, using all maps\n");
        for (int i = 0; i < mapManager_.GetCount(); i++) {
            modeMapIndices_.push_back(i);
        }
    }

    // 同步全局地图索引
    mapIndex_ = modeMapIndices_.empty() ? 0 : modeMapIndices_[modeMapIdx_];
}

void GameServer::AssignTeamForSlot(int slot) {
    GameMode mode = session_.GetGameMode();
    if (mode == GameMode::FREE_FOR_ALL) {
        // FFA: 每人独立队伍
        session_.GetSlot(slot).team = slot;
    } else {
        // 传统/攻防: 前两个一队, 后两个一队
        session_.GetSlot(slot).team = (slot < 2) ? 0 : 1;
    }
}

void GameServer::BroadcastLobbyState() {
    NetMessage state(NetMessageType::LobbyState);

    state.WritePayload(static_cast<uint16_t>(mapIndex_));
    state.WritePayload(static_cast<uint8_t>(static_cast<int>(session_.GetGameMode())));

    for (int i = 0; i < 4; i++) {
        const auto& slot = session_.GetSlot(i);
        state.WritePayload(static_cast<uint8_t>(slot.isHuman ? 1 : 0));
        state.WritePayload(static_cast<uint8_t>(static_cast<int>(slot.tankType)));
        state.WritePayload(static_cast<uint8_t>(slot.team));
        // Ready flag
        bool ready = false;
        if (slot.isHuman) {
            int ci = net_.FindClientBySocket(-1);  // need to find by slot
            for (const auto& c : net_.GetClients()) {
                if (c.slotIndex == i) { ready = c.ready; break; }
            }
        }
        state.WritePayload(static_cast<uint8_t>(ready ? 1 : 0));
    }

    net_.BroadcastTCP(state);
}

void GameServer::StartGame() {
    // Set map path from filtered mode maps
    int mapIdx = mapIndex_;
    if (mapIdx < 0 || mapIdx >= mapManager_.GetCount()) {
        mapIdx = modeMapIndices_.empty() ? 0 : modeMapIndices_[0];
    }
    session_.SetMapPath(mapManager_.GetMapInfo(mapIdx).filePath);
    printf("[SERVER] Starting game with map: %s (mode: %d)\n",
           session_.GetMapPath().c_str(), static_cast<int>(session_.GetGameMode()));

    // Randomize AI types
    session_.RandomizeAITypes();

    // Send GameStart to all clients
    NetMessage start(NetMessageType::GameStart);
    start.WriteString(session_.GetMapPath());
    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        const auto& slot = session_.GetSlot(i);
        start.WritePayload(static_cast<uint8_t>(slot.isHuman ? 1 : 0));
        start.WritePayload(static_cast<uint8_t>(static_cast<int>(slot.tankType)));
        start.WritePayload(static_cast<uint8_t>(slot.team));
    }
    net_.BroadcastTCP(start);

    // Load map
    map_.LoadFromFile(session_.GetMapPath());

    // Subscribe to events
    EventSystem::Instance().Subscribe(EventType::WallDestroyed, this);

    // Spawn bases
    SpawnBases();

    // Spawn tanks
    SpawnAllTanks();

    // Set up AI references
    tankPtrs_.clear();
    for (auto& t : tanks_) tankPtrs_.push_back(t.get());
    AIController::SetAllTanks(&tankPtrs_);
    AIController::SetBases(bases_[0], bases_[1]);

    // Initialize network host
    netHost_.SetNetwork(&net_);
    netHost_.Init(map_);

    // Reset game state
    gameOver_ = false;
    winningTeam_ = -1;
    gameOverTimer_ = 0.f;
    gameTimer_ = 0.f;
    stateTimer_ = 0.f;
    state_ = State::GAME;

    printf("[SERVER] Game started!\n");
}

// ============================================================
//  Game simulation (mirrors GameScene::Update without rendering)
// ============================================================

void GameServer::GameUpdate(float dt) {
    if (gameOver_) {
        gameOverTimer_ += dt;
        if (gameOverTimer_ < 0.1f) {
            netHost_.BroadcastGameOver(winningTeam_);
        }
        return;
    }

    gameTimer_ += dt;

    // Receive client input
    NetworkReceiveInput();

    // Update all tanks + detect death
    for (size_t i = 0; i < tanks_.size(); i++) {
        auto& tank = tanks_[i];
        bool wasDead = tank->IsDead();
        if (!wasDead) {
            tank->Update(dt);
            // Just died → handle lives
            if (tank->IsDead()) {
                auto& slot = session_.GetSlot(static_cast<int>(i));
                if (session_.GetGameMode() == GameMode::ATTACK_DEFEND) {
                    int team = slot.team;
                    if (team == 0) {
                        // 攻方: 从共享命池扣命, 即时重生
                        int& pool = session_.SharedLives(0);
                        if (pool > 0) {
                            pool--;
                            slot.isRespawning = true;
                            slot.respawnTimer = RESPAWN_DELAY_ATK;
                        }
                    } else {
                        // 守方: 无限命, 延迟重生
                        slot.isRespawning = true;
                        slot.respawnTimer = RESPAWN_DELAY_DEF;
                    }
                } else {
                    if (slot.lives > 0) {
                        slot.lives--;
                    }
                }
            }
        }
    }

    // Attack/Defend respawn system
    if (session_.GetGameMode() == GameMode::ATTACK_DEFEND) {
        UpdateRespawns(dt);
    }

    // Update entities
    entityManager_.Update(dt);

    // Collision detection
    CollisionSystem::Update(entityManager_, map_, dt);

    // Process pending power-ups
    for (auto& [x, y] : pendingPowerUps_) {
        SpawnPowerUp(x, y);
    }
    pendingPowerUps_.clear();

    // Cleanup
    entityManager_.RemoveInactive();

    // Check game over
    CheckGameOver();

    // Broadcast state at 20Hz
    stateTimer_ += dt;
    if (stateTimer_ >= NET_STATE_TICK_INTERVAL) {
        netHost_.BroadcastState(tanks_, entityManager_);
        netHost_.CheckMapChanges(map_);
        stateTimer_ = 0.f;
    }
}

// ============================================================
//  Spawn logic (from GameScene)
// ============================================================

void GameServer::SpawnBases() {
    baseCount_ = 0;
    bases_[0] = nullptr;
    bases_[1] = nullptr;

    GameMode mode = session_.GetGameMode();

    if (mode == GameMode::FREE_FOR_ALL) return;

    if (mode == GameMode::ATTACK_DEFEND) {
        Vector2 pos = map_.GetTeamBasePosition(1);
        bases_[1] = entityManager_.CreateEntity<Base>(pos, 1);
        baseCount_ = 1;

        auto* col = bases_[1]->GetComponent<ColliderComponent>();
        if (col) {
            col->collidesWith = CollisionLayer::Bullet;
            Base* base = bases_[1];
            col->onCollision = [base](Entity* other) {
                auto* bullet = dynamic_cast<Bullet*>(other);
                if (bullet && bullet->GetOwnerTeam() != base->GetTeam()) {
                    base->Destroy();
                    bullet->SetActive(false);
                }
            };
        }
        return;
    }

    // Traditional: both teams have bases
    for (int t = 0; t < 2; t++) {
        Vector2 pos = map_.GetTeamBasePosition(t);
        bases_[t] = entityManager_.CreateEntity<Base>(pos, t);

        auto* col = bases_[t]->GetComponent<ColliderComponent>();
        if (col) {
            col->collidesWith = CollisionLayer::Bullet;
            Base* base = bases_[t];
            col->onCollision = [base](Entity* other) {
                auto* bullet = dynamic_cast<Bullet*>(other);
                if (bullet && bullet->GetOwnerTeam() != base->GetTeam()) {
                    base->Destroy();
                    bullet->SetActive(false);
                }
            };
        }
    }
    baseCount_ = 2;
}

void GameServer::SpawnAllTanks() {
    netControllers_.clear();
    netControllers_.resize(4);

    GameMode mode = session_.GetGameMode();

    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        auto& slot = session_.GetSlot(i);
        int team = slot.team;

        std::unique_ptr<Tank> tank;
        if (slot.isHuman) {
            // All human players on the server use NetController
            auto nc = std::make_unique<NetController>(i);
            NetController* ncPtr = nc.get();
            tank = std::make_unique<Tank>(slot.tankType, std::move(nc), team);
            netControllers_[i] = ncPtr;  // raw pointer for input application
        } else {
            tank = TankFactory::CreateAITank(slot.tankType, team);
        }

        // Spawn position
        Vector2 spawnPos;
        if (mode == GameMode::FREE_FOR_ALL) {
            const auto& spawns = map_.GetSpawnPoints();
            spawnPos = (i < static_cast<int>(spawns.size()))
                ? spawns[i]
                : Vector2{ TILE_SIZE * 3.f, TILE_SIZE * 3.f };
        } else {
            const auto& spawns = map_.GetTeamSpawnPoints(team);
            int spawnIdx = i % static_cast<int>(spawns.size());
            spawnPos = spawns.empty()
                ? Vector2{ TILE_SIZE * 3.f, TILE_SIZE * 3.f }
                : spawns[spawnIdx];
        }

        tank->Spawn(entityManager_, spawnPos);
        slot.tank = tank.get();
        tanks_.push_back(std::move(tank));
    }
}

// ============================================================
//  Power-ups
// ============================================================

void GameServer::SpawnPowerUp(float x, float y) {
    int typeIdx = Random::Int(0, 4);
    auto type = static_cast<PowerUpType>(typeIdx);
    auto* pu = entityManager_.CreateEntity<PowerUp>(type, Vector2{ x, y });
    SetupPowerUpCollision(pu);
}

void GameServer::SetupPowerUpCollision(PowerUp* pu) {
    auto* col = pu->GetComponent<ColliderComponent>();
    if (!col) return;

    col->collidesWith = CollisionLayer::Player;
    GameServer* server = this;
    PowerUpType puType = pu->GetType();

    col->onCollision = [server, puType, pu](Entity* other) {
        auto* collector = server->FindTankByEntity(other);
        if (!collector || collector->IsDead()) return;

        switch (puType) {
            case PowerUpType::STAR:
                collector->SetInvincible(5.f);
                break;
            case PowerUpType::HELMET:
                collector->SetInvincible(8.f);
                break;
            case PowerUpType::BOMB:
                for (auto& t : server->tanks_) {
                    if (t->GetTeam() != collector->GetTeam() && !t->IsDead()) {
                        t->TakeDamage(999);
                    }
                }
                break;
            default:
                break;
        }

        pu->SetActive(false);
    };
}

// ============================================================
//  Game over detection
// ============================================================

void GameServer::CheckGameOver() {
    GameMode mode = session_.GetGameMode();

    if (mode == GameMode::TRADITIONAL) {
        for (int t = 0; t < 2; t++) {
            if (bases_[t] && bases_[t]->IsDestroyed()) {
                gameOver_ = true;
                winningTeam_ = 1 - t;
                printf("[SERVER] Game over! Team %d wins (base destroyed)\n", winningTeam_);
                return;
            }
        }
        for (int t = 0; t < 2; t++) {
            if (GetAliveCount(t) == 0) {
                gameOver_ = true;
                winningTeam_ = 1 - t;
                printf("[SERVER] Game over! Team %d wins (elimination)\n", winningTeam_);
                return;
            }
        }
    } else if (mode == GameMode::ATTACK_DEFEND) {
        if (bases_[1] && bases_[1]->IsDestroyed()) {
            gameOver_ = true;
            winningTeam_ = 0;
            printf("[SERVER] Game over! Attackers win (base destroyed)\n");
            return;
        }

        // 守方胜利: 攻方共享命用完且所有攻方玩家死亡
        if (session_.GetSharedLives(0) <= 0) {
            bool allAttackersDead = true;
            for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
                if (session_.GetSlot(i).team == 0 && !tanks_[i]->IsDead()) {
                    allAttackersDead = false;
                    break;
                }
            }
            if (allAttackersDead) {
                gameOver_ = true;
                winningTeam_ = 1;
                printf("[SERVER] Game over! Defenders win\n");
                return;
            }
        }
    } else if (mode == GameMode::FREE_FOR_ALL) {
        int aliveCount = 0;
        int lastAlive = -1;
        for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
            if (!tanks_[i]->IsDead()) {
                aliveCount++;
                lastAlive = i;
            }
        }
        if (aliveCount <= 1) {
            gameOver_ = true;
            winningTeam_ = lastAlive;
            if (lastAlive >= 0)
                printf("[SERVER] Game over! Player %d wins (FFA)\n", lastAlive + 1);
            else
                printf("[SERVER] Game over! Draw\n");
            return;
        }
    }
}

int GameServer::GetAliveCount(int team) const {
    int count = 0;
    for (auto& tank : tanks_) {
        if (tank->GetTeam() == team && !tank->IsDead()) {
            count++;
        }
    }
    return count;
}

// ============================================================
//  Event handling
// ============================================================

void GameServer::OnEvent(const Event& event) {
    if (event.type == EventType::WallDestroyed) {
        if (Random::Float(0.f, 1.f) < 0.1f) {
            pendingPowerUps_.emplace_back(event.x, event.y);
        }
    }
}

// ============================================================
//  Helpers
// ============================================================

Tank* GameServer::FindTankByEntity(Entity* e) {
    for (auto& t : tanks_) {
        if (t->GetEntity() == e) return t.get();
    }
    return nullptr;
}

void GameServer::NetworkReceiveInput() {
    for (auto& client : net_.GetClients()) {
        NetMessage m;
        while (net_.ReceiveFromClient(client.socket, m)) {
            if (m.header.type == NetMessageType::InputState) {
                auto input = m.ReadPayload<NetInputPayload>();
                int slot = client.slotIndex;
                if (slot >= 0 && slot < 4 && netControllers_[slot]) {
                    netControllers_[slot]->ApplyInput(input.inputMask);
                }
            }
        }
    }
}

void GameServer::UpdateRespawns(float dt) {
    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        auto& slot = session_.GetSlot(i);
        if (!slot.isRespawning) continue;

        slot.respawnTimer -= dt;
        if (slot.respawnTimer <= 0.f) {
            slot.isRespawning = false;

            auto oldTank = std::move(tanks_[i]);

            const auto& spawns = map_.GetTeamSpawnPoints(slot.team);
            Vector2 spawnPos = spawns.empty()
                ? Vector2{ TILE_SIZE * 3.f, TILE_SIZE * 3.f }
                : spawns[Random::Int(0, static_cast<int>(spawns.size()) - 1)];

            std::unique_ptr<Tank> newTank;
            if (slot.isHuman) {
                newTank = TankFactory::CreatePlayerTank(
                    oldTank->GetType(), slot.team,
                    slot.keyUp, slot.keyDown, slot.keyLeft, slot.keyRight, slot.keyFire);
            } else {
                newTank = TankFactory::CreateAITank(oldTank->GetType(), slot.team);
            }

            newTank->Spawn(entityManager_, spawnPos);
            newTank->SetInvincible(RESPAWN_INVINCIBLE);

            slot.tank = newTank.get();
            tanks_[i] = std::move(newTank);

            // Update AI references
            tankPtrs_.clear();
            for (auto& t : tanks_) tankPtrs_.push_back(t.get());
            AIController::SetAllTanks(&tankPtrs_);
        }
    }
}

void GameServer::CleanupGame() {
    EventSystem::Instance().Unsubscribe(EventType::WallDestroyed, this);

    tanks_.clear();
    tankPtrs_.clear();
    netControllers_.clear();
    bases_[0] = nullptr;
    bases_[1] = nullptr;
    baseCount_ = 0;
    pendingPowerUps_.clear();
    entityManager_ = EntityManager();

    // Reset slots to AI
    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        auto& slot = session_.GetSlot(i);
        slot.tank = nullptr;
        slot.lives = DEFAULT_LIVES;
        slot.isRespawning = false;
        slot.respawnTimer = 0.f;
        // Keep isHuman/team/tankType as set in lobby
    }

    // Reset all client ready flags
    for (auto& client : net_.GetClients()) {
        client.ready = false;
    }
    lobbyTimer_ = 0.f;
    heartbeatTimer_ = 0.f;

    printf("[SERVER] Game state cleaned up\n");
}
