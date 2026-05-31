// winsock2.h 必须在 windows.h (raylib) 之前包含
#include "net/NetworkManager.h"
#include "net/NetController.h"

#include "GameServer.h"
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

GameServer::GameServer() = default;
GameServer::~GameServer() {
    Stop();
}

bool GameServer::Start(uint16_t port) {
    if (!net_.Listen(port)) {
        printf("[SERVER] Failed to listen on port %d\n", port);
        return false;
    }

    // Scan maps for Traditional mode (default)
    mapManager_.ScanMapsForMode("assets/maps", GameMode::TRADITIONAL);
    if (mapManager_.GetCount() > 0) {
        mapIndex_ = 0;
    }

    session_.SetGameMode(GameMode::TRADITIONAL);

    printf("[SERVER] Listening on port %d\n", port);
    printf("[SERVER] Waiting for players to connect...\n");
    printf("[SERVER] Type 'start' to begin the game.\n");

    running_ = true;
    state_ = State::LOBBY;
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

    // Find an available slot (skip slots already taken by connected clients)
    int slot = -1;
    bool used[4] = {};
    for (auto& c : net_.GetClients()) {
        if (c.slotIndex >= 0 && c.slotIndex < 4)
            used[c.slotIndex] = true;
    }
    for (int i = 0; i < 4; i++) {
        if (!used[i]) { slot = i; break; }
    }

    if (slot < 0) {
        // Server full
        NetMessage kick(NetMessageType::Kick);
        kick.WriteString("Server full");
        net_.SendTo(newSock, kick);
        net_.DisconnectClient(newSock);
        printf("[SERVER] Rejected connection: server full\n");
        return;
    }

    // Assign slot
    auto& clients = net_.GetClients();
    clients.back().slotIndex = slot;
    session_.GetSlot(slot).isHuman = true;

    // Send acceptance
    NetMessage accepted(NetMessageType::JoinAccepted);
    accepted.WritePayload(static_cast<uint8_t>(slot));
    net_.SendTo(newSock, accepted);

    // Broadcast new player joined
    NetMessage joined(NetMessageType::PlayerJoined);
    joined.WritePayload(static_cast<uint8_t>(slot));
    joined.WriteString("Player");
    net_.BroadcastTCP(joined);

    BroadcastLobbyState();

    printf("[SERVER] Player joined as P%d\n", slot + 1);
}

// ============================================================
//  Lobby
// ============================================================

void GameServer::LobbyUpdate(float dt) {
    lobbyTimer_ += dt;

    // Process messages from all clients
    for (auto& client : net_.GetClients()) {
        NetMessage msg;
        while (net_.ReceiveFromClient(client.socket, msg)) {
            HandleLobbyMessage(client.socket, msg);
        }
    }

    // Auto-start: after 10s with at least 1 player, or immediately if flag set
    if (gameStartRequested_) {
        gameStartRequested_ = false;
        StartGame();
    } else if (net_.ClientCount() > 0 && lobbyTimer_ > 10.f) {
        printf("[SERVER] Auto-starting game (lobby timeout)\n");
        StartGame();
    }
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
            }
            BroadcastLobbyState();
            break;
        }
        case NetMessageType::Ping: {
            NetMessage pong(NetMessageType::Pong);
            net_.SendTo(clientSocket, pong);
            break;
        }
        default:
            break;
    }
}

void GameServer::BroadcastLobbyState() {
    NetMessage state(NetMessageType::LobbyState);

    state.WritePayload(static_cast<uint16_t>(mapIndex_));

    for (int i = 0; i < 4; i++) {
        const auto& slot = session_.GetSlot(i);
        state.WritePayload(static_cast<uint8_t>(slot.isHuman ? 1 : 0));
        state.WritePayload(static_cast<uint8_t>(static_cast<int>(slot.tankType)));
        state.WritePayload(static_cast<uint8_t>(slot.team));
    }

    net_.BroadcastTCP(state);
}

void GameServer::StartGame() {
    // Set map path
    if (mapIndex_ >= 0 && mapIndex_ < mapManager_.GetCount()) {
        session_.SetMapPath(mapManager_.GetMapInfo(mapIndex_).filePath);
    } else {
        session_.SetMapPath("assets/maps/traditional/classic.txt");
    }

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
    printf("[SERVER] Starting game with map: %s\n", session_.GetMapPath().c_str());
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
            if (tank->IsDead() && tank->GetEntity()) {
                auto& slot = session_.GetSlot(static_cast<int>(i));
                if (slot.lives > 0) {
                    slot.lives--;
                }
                // Attack/Defend: mark defender respawn
                if (session_.GetGameMode() == GameMode::ATTACK_DEFEND
                    && slot.team == 1 && slot.lives < 0) {
                    slot.isRespawning = true;
                    slot.respawnTimer = RESPAWN_DELAY;
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
                if (bullet) {
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
                if (bullet) {
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

        bool allAttackersDead = true;
        for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
            auto& slot = session_.GetSlot(i);
            if (slot.team == 0) {
                if (slot.lives > 0 || !tanks_[i]->IsDead()) {
                    allAttackersDead = false;
                    break;
                }
            }
        }
        if (allAttackersDead) {
            gameOver_ = true;
            winningTeam_ = 1;
            printf("[SERVER] Game over! Defenders win\n");
            return;
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

    printf("[SERVER] Game state cleaned up\n");
}
