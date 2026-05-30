// winsock2.h 必须在 windows.h (raylib) 之前包含
// NetworkManager.h 必须在 raylib (Types.h) 之前包含
#include "net/NetworkManager.h"
#include "net/NetController.h"

#include "GameScene.h"
#include "SceneManager.h"
#include "PauseScene.h"
#include "GameOverScene.h"
#include "ecs/components/HealthComponent.h"
#include "game/tank/TankFactory.h"
#include "game/tank/controller/AIController.h"
#include "game/entities/Bullet.h"
#include "game/systems/CollisionSystem.h"
#include "ecs/components/ColliderComponent.h"
#include "ecs/components/TransformComponent.h"
#include "core/EventSystem.h"
#include "core/AudioManager.h"
#include "core/Types.h"
#include "utils/Random.h"
#include <cstdio>

void GameScene::SetNetworkManager(NetworkManager* net) {
    ownedNet_.reset(net);
    net_ = net;
    netHost_.SetNetwork(net);
}

GameScene::GameScene(SceneManager* manager, GameSession session)
    : manager_(manager), session_(std::move(session)),
      mode_(session_.GetGameMode()), hud_(session_) {}

GameScene::~GameScene() = default;

void GameScene::Enter() {
    // 游戏开始时随机分配 AI 坦克类型
    session_.RandomizeAITypes();

    // 加载地图
    map_.LoadFromFile(session_.GetMapPath());

    // 根据模式生成基地
    SpawnBases();

    // 生成所有坦克
    SpawnAllTanks();

    // 设置 AI 控制器的引用
    tankPtrs_.clear();
    for (auto& t : tanks_) tankPtrs_.push_back(t.get());
    AIController::SetAllTanks(&tankPtrs_);
    AIController::SetBases(bases_[0], bases_[1]);

    // 订阅事件 (砖墙破坏时可能掉落道具)
    EventSystem::Instance().Subscribe(EventType::WallDestroyed, this);

    // 网络初始化
    if (net_) netHost_.Init(map_);
}

void GameScene::Exit() {
    EventSystem::Instance().Unsubscribe(EventType::WallDestroyed, this);

    // 网络清理: 关闭连接 (但不释放 ownedNet_, 让析构函数处理)
    if (net_) {
        net_->DisconnectAllClients();
        net_->Close();
        net_->CloseUDP();
        net_ = nullptr;
    }
}

void GameScene::Update(float dt) {
    if (gameOver_) {
        gameOverTimer_ += dt;

        // 网络 Host: 广播游戏结束
        if (net_ && gameOverTimer_ < 0.1f) {
            netHost_.BroadcastGameOver(winningTeam_);
        }

        if (gameOverTimer_ > 2.f) {
            if (net_) {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                    manager_->ReturnToMenu();
                }
                return;
            }

            // 热座模式: 进入结算场景
            manager_->SetupNext(
                std::make_unique<GameOverScene>(manager_, winningTeam_,
                                                std::move(session_)));
        }
        return;
    }

    // 暂停
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
        manager_->PushScene(std::make_unique<PauseScene>(manager_));
        return;
    }

    // 游戏计时
    gameTimer_ += dt;

    // 网络 Host: 接收客户端输入
    if (net_) NetworkReceiveInput();

    // 更新所有坦克 + 检测死亡
    for (size_t i = 0; i < tanks_.size(); i++) {
        auto& tank = tanks_[i];
        bool wasDead = tank->IsDead();
        if (!wasDead) {
            tank->Update(dt);
            // 刚死亡 → 爆炸特效 + 处理命
            if (tank->IsDead() && tank->GetEntity()) {
                auto* t = tank->GetEntity()->GetComponent<TransformComponent>();
                if (t) {
                    Color c = tank->GetTeam() == 0 ? RED : BLUE;
                    if (mode_ == GameMode::FREE_FOR_ALL) {
                        static Color ffaColors[] = { RED, BLUE, GREEN, PURPLE };
                        c = (i < 4) ? ffaColors[i] : WHITE;
                    }
                    particles_.Emit(t->GetPosition(), 20, c, 150.f);
                }

                // 扣命
                auto& slot = session_.GetSlot(static_cast<int>(i));
                if (slot.lives > 0) {
                    slot.lives--;
                }
                // 标记攻防战守方重生
                if (mode_ == GameMode::ATTACK_DEFEND && slot.team == 1 && slot.lives < 0) {
                    slot.isRespawning = true;
                    slot.respawnTimer = RESPAWN_DELAY;
                }
            }
        }
    }

    // 攻防战: 处理守方重生
    if (mode_ == GameMode::ATTACK_DEFEND) {
        UpdateRespawns(dt);
    }

    // 更新粒子
    particles_.Update(dt);

    // 更新实体管理器
    entityManager_.Update(dt);

    // 碰撞检测 (dt 用于墙碰撞回退量计算)
    CollisionSystem::Update(entityManager_, map_, dt);

    // 处理延迟生成的道具
    for (auto& [x, y] : pendingPowerUps_) {
        SpawnPowerUp(x, y);
    }
    pendingPowerUps_.clear();

    // 清理不活跃实体
    entityManager_.RemoveInactive();

    // 检查游戏结束条件
    CheckGameOver();

    // 网络 Host: 广播状态 + 地图变化
    if (net_) {
        static float netTimer = 0.f;
        netTimer += dt;
        if (netTimer >= NET_STATE_TICK_INTERVAL) {
            netHost_.BroadcastState(tanks_, entityManager_);
            netHost_.CheckMapChanges(map_);
            netTimer = 0.f;
        }
    }
}

void GameScene::Render() {
    // 渲染地图
    map_.Render();

    // 渲染实体 (子弹等)
    entityManager_.Render();

    // 渲染基地 (仅在有基地的模式下)
    for (int i = 0; i < baseCount_; i++) {
        if (bases_[i]) bases_[i]->Render();
    }

    // 渲染坦克 (死亡且不在重生中的才不渲染)
    for (size_t i = 0; i < tanks_.size(); i++) {
        auto& tank = tanks_[i];
        if (!tank->IsDead()) {
            tank->Render();
        } else if (mode_ == GameMode::ATTACK_DEFEND) {
            // 攻防战: 正在重生的守方显示半透明
            auto& slot = session_.GetSlot(static_cast<int>(i));
            if (slot.isRespawning && slot.team == 1) {
                float alpha = 0.3f + 0.2f * std::sin(gameTimer_ * 8.f);
                // 简单方案: 在重生点画一个闪烁的矩形
                auto* t = tank->GetEntity();
                if (t) {
                    auto* transform = t->GetComponent<TransformComponent>();
                    if (transform) {
                        Vector2 pos = transform->GetPosition();
                        DrawRectangle(
                            static_cast<int>(pos.x - TILE_SIZE * 0.4f),
                            static_cast<int>(pos.y - TILE_SIZE * 0.4f),
                            static_cast<int>(TILE_SIZE * 0.8f),
                            static_cast<int>(TILE_SIZE * 0.8f),
                            ColorAlpha(BLUE, alpha));
                    }
                }
            }
        }
    }

    // 渲染粒子
    particles_.Render();

    // 渲染 HUD
    hud_.Render();

    // 攻防战: 显示游戏计时器
    if (mode_ == GameMode::ATTACK_DEFEND && !gameOver_) {
        int minutes = static_cast<int>(gameTimer_) / 60;
        int seconds = static_cast<int>(gameTimer_) % 60;
        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", minutes, seconds);
        int tw = MeasureText(timeBuf, 24);
        DrawText(timeBuf, (GetScreenWidth() - tw) / 2, 10, 24, YELLOW);
    }

    // 游戏结束叠加文字
    if (gameOver_) {
        DrawRectangle(0, 300, GetScreenWidth(), 100, ColorAlpha(BLACK, 0.6f));
        const char* text = nullptr;
        Color color = WHITE;

        if (mode_ == GameMode::FREE_FOR_ALL) {
            static const char* ffaNames[] = { "RED", "BLUE", "GREEN", "PURPLE" };
            static Color ffaColors[] = { RED, BLUE, GREEN, PURPLE };
            static char ffaBuf[64];
            if (winningTeam_ >= 0 && winningTeam_ < 4) {
                snprintf(ffaBuf, sizeof(ffaBuf), "%s PLAYER WINS!", ffaNames[winningTeam_]);
                text = ffaBuf;
                color = ffaColors[winningTeam_];
            } else {
                text = "DRAW!";
            }
        } else if (mode_ == GameMode::ATTACK_DEFEND) {
            text = (winningTeam_ == 0) ? "ATTACK WINS!" : "DEFEND WINS!";
            color = (winningTeam_ == 0) ? RED : BLUE;
        } else {
            text = (winningTeam_ == 0) ? "RED WINS!" : "BLUE WINS!";
            color = (winningTeam_ == 0) ? RED : BLUE;
        }

        int w = MeasureText(text, 50);
        DrawText(text, (GetScreenWidth() - w) / 2, 340, 50, color);
    }
}

void GameScene::SpawnBases() {
    baseCount_ = 0;
    bases_[0] = nullptr;
    bases_[1] = nullptr;

    if (mode_ == GameMode::FREE_FOR_ALL) {
        // FFA: 无基地
        return;
    }

    if (mode_ == GameMode::ATTACK_DEFEND) {
        // 攻防战: 只有守方 (team 1) 有基地
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

    // 传统对战: 双方都有基地
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

void GameScene::SpawnAllTanks() {
    netControllers_.resize(4, nullptr);

    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        auto& slot = session_.GetSlot(i);
        int team = slot.team;

        // 创建坦克
        std::unique_ptr<Tank> tank;
        if (slot.isHuman) {
            if (net_ && i > 0) {
                auto nc = std::make_unique<NetController>(i);
                netControllers_[i] = nc.get();
                tank = std::make_unique<Tank>(slot.tankType, std::move(nc), team);
            } else {
                tank = TankFactory::CreatePlayerTank(
                    slot.tankType, team,
                    slot.keyUp, slot.keyDown, slot.keyLeft, slot.keyRight, slot.keyFire);
            }
        } else {
            tank = TankFactory::CreateAITank(slot.tankType, team);
        }

        // 出生点选择
        Vector2 spawnPos;
        if (mode_ == GameMode::FREE_FOR_ALL) {
            // FFA: 使用 4 角出生点
            const auto& spawns = map_.GetSpawnPoints();
            spawnPos = (i < static_cast<int>(spawns.size()))
                ? spawns[i]
                : Vector2{ TILE_SIZE * 3.f, TILE_SIZE * 3.f };
        } else {
            // 传统/攻防: 使用队伍出生点
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

void GameScene::CheckGameOver() {
    if (mode_ == GameMode::TRADITIONAL) {
        // 传统对战: 摧毁敌方基地 或 全灭敌方
        for (int t = 0; t < 2; t++) {
            if (bases_[t] && bases_[t]->IsDestroyed()) {
                gameOver_ = true;
                winningTeam_ = 1 - t;
                return;
            }
        }
        for (int t = 0; t < 2; t++) {
            if (GetAliveCount(t) == 0) {
                gameOver_ = true;
                winningTeam_ = 1 - t;
                return;
            }
        }
    } else if (mode_ == GameMode::ATTACK_DEFEND) {
        // 攻防战:
        // 攻方胜利: 摧毁守方基地
        if (bases_[1] && bases_[1]->IsDestroyed()) {
            gameOver_ = true;
            winningTeam_ = 0;  // 攻方赢
            return;
        }

        // 守方胜利: 所有攻方玩家命用完且死亡
        bool allAttackersDead = true;
        for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
            auto& slot = session_.GetSlot(i);
            if (slot.team == 0) {  // 攻方
                if (slot.lives > 0 || !tanks_[i]->IsDead()) {
                    allAttackersDead = false;
                    break;
                }
            }
        }
        if (allAttackersDead) {
            gameOver_ = true;
            winningTeam_ = 1;  // 守方赢
            return;
        }
    } else if (mode_ == GameMode::FREE_FOR_ALL) {
        // 各自为战: 最后存活者获胜
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
            winningTeam_ = lastAlive;  // 获胜玩家索引, -1=平局
            return;
        }
    }
}

int GameScene::GetAliveCount(int team) const {
    int count = 0;
    for (auto& tank : tanks_) {
        if (tank->GetTeam() == team && !tank->IsDead()) {
            count++;
        }
    }
    return count;
}

void GameScene::OnEvent(const Event& event) {
    if (event.type == EventType::WallDestroyed) {
        if (Random::Float(0.f, 1.f) < 0.1f) {
            pendingPowerUps_.emplace_back(event.x, event.y);
        }
    }
}

Tank* GameScene::FindTankByEntity(Entity* e) {
    for (auto& t : tanks_) {
        if (t->GetEntity() == e) return t.get();
    }
    return nullptr;
}

void GameScene::SpawnPowerUp(float x, float y) {
    // 随机道具类型
    int typeIdx = Random::Int(0, 4);
    auto type = static_cast<PowerUpType>(typeIdx);

    auto* pu = entityManager_.CreateEntity<PowerUp>(type, Vector2{ x, y });
    SetupPowerUpCollision(pu);
}

void GameScene::SetupPowerUpCollision(PowerUp* pu) {
    auto* col = pu->GetComponent<ColliderComponent>();
    if (!col) return;

    col->collidesWith = CollisionLayer::Player;
    GameScene* scene = this;
    PowerUpType puType = pu->GetType();

    col->onCollision = [scene, puType, pu](Entity* other) {
        auto* collector = scene->FindTankByEntity(other);
        if (!collector || collector->IsDead()) return;

        switch (puType) {
            case PowerUpType::STAR:
                collector->SetInvincible(5.f);
                break;
            case PowerUpType::HELMET:
                collector->SetInvincible(8.f);
                break;
            case PowerUpType::BOMB:
                for (auto& t : scene->tanks_) {
                    if (t->GetTeam() != collector->GetTeam() && !t->IsDead()) {
                        t->TakeDamage(999);
                    }
                }
                break;
            default:
                break;
        }

        AudioManager::Instance().PlaySound("pickup");
        pu->SetActive(false);
    };
}

// ============================================================
//  攻防战重生系统
// ============================================================

void GameScene::UpdateRespawns(float dt) {
    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        auto& slot = session_.GetSlot(i);
        if (!slot.isRespawning) continue;

        slot.respawnTimer -= dt;
        if (slot.respawnTimer <= 0.f) {
            // 重生守方坦克
            slot.isRespawning = false;

            // 重新创建坦克实体
            auto oldTank = std::move(tanks_[i]);

            // 从队伍出生点中随机选取
            const auto& spawns = map_.GetTeamSpawnPoints(slot.team);
            Vector2 spawnPos = spawns.empty()
                ? Vector2{ TILE_SIZE * 3.f, TILE_SIZE * 3.f }
                : spawns[Random::Int(0, static_cast<int>(spawns.size()) - 1)];

            // 创建新坦克 (保留类型)
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

            // 更新 AI 引用
            tankPtrs_.clear();
            for (auto& t : tanks_) tankPtrs_.push_back(t.get());
            AIController::SetAllTanks(&tankPtrs_);
        }
    }
}

// ============================================================
//  网络 Host: 输入接收 (需要访问 netControllers_)
// ============================================================

void GameScene::NetworkReceiveInput() {
    if (!net_) return;

    // 逐个客户端接收 TCP 输入
    for (auto& client : net_->GetClients()) {
        NetMessage m;
        while (net_->ReceiveFromClient(client.socket, m)) {
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

