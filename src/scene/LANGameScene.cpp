// winsock2.h 必须在 windows.h (raylib) 之前包含
#include "net/NetworkManager.h"

#include "LANGameScene.h"
#include "SceneManager.h"
#include "game/tank/TankType.h"
#include "game/map/TileType.h"
#include "core/Types.h"
#include "core/ResourceManager.h"
#include "raylib.h"
#include <cstring>
#include <cmath>

LANGameScene::LANGameScene(SceneManager* manager, NetworkManager* net, GameSession session, int mySlot)
    : manager_(manager), ownedNet_(net), net_(net), session_(std::move(session)),
      mySlot_(mySlot), mode_(session_.GetGameMode())
{
    players_.resize(4);
    for (int i = 0; i < 4; i++) {
        players_[i].type = session_.GetSlot(i).tankType;
        players_[i].team = session_.GetSlot(i).team;
    }
}

LANGameScene::~LANGameScene() = default;

void LANGameScene::Enter() {
    TraceLog(LOG_INFO, "LANGameScene::Enter() - begin");
    TraceLog(LOG_INFO, "LANGameScene::Enter() - loading map: %s", session_.GetMapPath().c_str());
    map_.LoadFromFile(session_.GetMapPath());
    gameOver_ = false;
    winningTeam_ = -1;
    gameStarted_ = true;
    TraceLog(LOG_INFO, "LANGameScene::Enter() - done");
    disconnectTimer_ = 0.f;
    inputTimer_ = 0.f;

    if (net_) {
        net_->BindUDP(NET_DEFAULT_PORT + 1);
    }
}

void LANGameScene::Exit() {
    if (net_) {
        net_->Disconnect();
        net_->Close();
        net_->CloseUDP();
        net_ = nullptr;
    }
}

void LANGameScene::Update(float dt) {
    inputTimer_ += dt;
    disconnectTimer_ += dt;

    if (gameOver_) {
        gameOverTimer_ += dt;
        if (gameOverTimer_ > 2.f) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                manager_->PostReturnToMenu();
            }
        }
        return;
    }

    try {
        ProcessMessages();
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Network error in ProcessMessages: %s", e.what());
        manager_->PostReturnToMenu();
        return;
    } catch (...) {
        TraceLog(LOG_ERROR, "Unknown network error in ProcessMessages");
        manager_->PostReturnToMenu();
        return;
    }

    // 发送输入 (~60Hz)
    if (inputTimer_ >= NET_INPUT_TICK_INTERVAL) {
        try {
            SendInput();
        } catch (...) {
            TraceLog(LOG_ERROR, "Network error in SendInput");
            manager_->PostReturnToMenu();
            return;
        }
        inputTimer_ = 0.f;
    }

    // 超时检测: Host/Server 可能已断开
    if (disconnectTimer_ > NET_TIMEOUT_SECONDS * 2.f) {
        TraceLog(LOG_WARNING, "Connection timeout, returning to menu");
        manager_->PostReturnToMenu();
    }

    // ESC: 断开连接并退出
    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->PostReturnToMenu();
    }
}

void LANGameScene::ProcessMessages() {
    if (!net_ || !net_->IsConnected()) return;

    // TCP 消息
    NetMessage msg;
    while (net_->Receive(msg)) {
        disconnectTimer_ = 0.f;

        switch (msg.header.type) {
            case NetMessageType::GameState:
                ParseGameState(msg);
                break;
            case NetMessageType::WallChanged: {
                auto wc = msg.ReadPayload<NetWallChangedPayload>();
                map_.SetTile(wc.col, wc.row, static_cast<TileType>(wc.newType));
                break;
            }
            case NetMessageType::GameOver: {
                uint8_t winner = msg.ReadPayload<uint8_t>();
                gameOver_ = true;
                winningTeam_ = winner;
                gameOverTimer_ = 0.f;
                break;
            }
            case NetMessageType::ReturnToLobby:
                manager_->PostReturnToMenu();
                return;
            default: break;
        }
    }

    // UDP 消息 (仅 GameState)
    NetMessage udpMsg;
    while (net_->ReceiveUDP(udpMsg)) {
        disconnectTimer_ = 0.f;
        if (udpMsg.header.type == NetMessageType::GameState) {
            ParseGameState(udpMsg);
        }
    }
}

void LANGameScene::ParseGameState(const NetMessage& msg) {
    std::size_t offset = 0;

    uint16_t seq = msg.ReadPayload<uint16_t>(offset);
    offset += 2;
    if (seq <= lastStateSeq_) return;  // 丢弃乱序包
    lastStateSeq_ = seq;

    // 4 个玩家状态
    for (int i = 0; i < 4; i++) {
        auto& p = players_[i];
        p.x          = msg.ReadPayload<float>(offset);   offset += 4;
        p.y          = msg.ReadPayload<float>(offset);   offset += 4;
        p.rotation   = msg.ReadPayload<float>(offset);   offset += 4;
        p.hp         = msg.ReadPayload<uint8_t>(offset); offset++;
        p.maxHp      = msg.ReadPayload<uint8_t>(offset); offset++;
        p.alive      = msg.ReadPayload<uint8_t>(offset) != 0; offset++;
        p.invincible = msg.ReadPayload<uint8_t>(offset) != 0; offset++;
    }

    // 子弹列表
    uint8_t bulletCount = msg.ReadPayload<uint8_t>(offset);
    offset++;
    bullets_.clear();
    for (int b = 0; b < bulletCount && offset + sizeof(NetBulletState) <= msg.payload.size(); b++) {
        auto bs = msg.ReadPayload<NetBulletState>(offset);
        offset += sizeof(NetBulletState);
        RemoteBullet rb;
        rb.id        = bs.bulletId;
        rb.x         = bs.x;
        rb.y         = bs.y;
        rb.ownerTeam = bs.ownerTeam;
        rb.active    = bs.active != 0;
        if (rb.active) bullets_.push_back(rb);
    }
}

void LANGameScene::SendInput() {
    if (!net_ || !net_->IsConnected()) return;

    uint8_t mask = 0;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    mask |= NET_INPUT_UP;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))   mask |= NET_INPUT_DOWN;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))   mask |= NET_INPUT_LEFT;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))  mask |= NET_INPUT_RIGHT;
    if (IsKeyDown(KEY_SPACE))                       mask |= NET_INPUT_FIRE;

    NetMessage input(NetMessageType::InputState);
    NetInputPayload payload;
    payload.seq = lastStateSeq_;
    payload.inputMask = mask;
    input.WritePayload(payload);

    // 通过 TCP 发送输入 (简单可靠)
    net_->Send(input);
}

void LANGameScene::Render() {
    ClearBackground(BLACK);

    if (!gameStarted_) {
        const char* waiting = "Waiting for game state...";
        DrawText(waiting, (GetScreenWidth() - MeasureText(waiting, 24)) / 2, 400, 24, WHITE);
        return;
    }

    // 渲染地图
    map_.Render();

    // 渲染坦克
    RenderPlayers();

    // 渲染子弹
    RenderBullets();

    // 渲染 HUD
    RenderHUD();

    // 游戏结束
    if (gameOver_) {
        DrawRectangle(0, 300, GetScreenWidth(), 100, ColorAlpha(BLACK, 0.6f));
        const char* text = "WINS!";
        Color color = RED;
        if (mode_ == GameMode::ATTACK_DEFEND) {
            text = (winningTeam_ == 0) ? "ATTACK WINS!" : "DEFEND WINS!";
            color = (winningTeam_ == 0) ? RED : BLUE;
        } else if (mode_ == GameMode::FREE_FOR_ALL) {
            static const char* ffaColors[] = { "RED", "BLUE", "GREEN", "PURPLE" };
            static Color ffaColorVals[] = { RED, BLUE, GREEN, PURPLE };
            text = "PLAYER WINS!";
            color = (winningTeam_ >= 0 && winningTeam_ < 4) ? ffaColorVals[winningTeam_] : WHITE;
        } else {
            text = (winningTeam_ == 0) ? "RED WINS!" : "BLUE WINS!";
            color = (winningTeam_ == 0) ? RED : BLUE;
        }
        int w = MeasureText(text, 50);
        DrawText(text, (GetScreenWidth() - w) / 2, 340, 50, color);

        if (gameOverTimer_ > 2.f) {
            const char* hint = "ENTER/ESC = Return to Menu";
            DrawText(hint, (GetScreenWidth() - MeasureText(hint, 18)) / 2, 420, 18, GRAY);
        }
    }
}

void LANGameScene::RenderPlayers() {
    static const Color teamColors[] = { RED, BLUE, GREEN, PURPLE };
    static const char* typeNames[] = { "light", "medium", "heavy", "speed" };
    auto& rm = ResourceManager::Instance();

    for (int i = 0; i < 4; i++) {
        const auto& p = players_[i];
        if (!p.alive) continue;

        Color teamColor = (p.team >= 0 && p.team < 4) ? teamColors[p.team] : WHITE;
        Color tint = WHITE;

        // 无敌闪烁
        if (p.invincible) {
            float alpha = std::abs(std::sin(GetTime() * 10.f));
            tint = ColorAlpha(WHITE, alpha);
        }

        // 绘制坦克纹理
        int typeIdx = static_cast<int>(p.type);
        if (typeIdx < 0 || typeIdx > 3) typeIdx = 1;
        std::string texName = std::string("tank_") + typeNames[typeIdx] + "_team" + std::to_string(p.team);

        if (rm.HasTexture(texName)) {
            Texture2D& tex = rm.GetTexture(texName);
            float scale = TILE_SIZE / static_cast<float>(tex.width);
            float origin = tex.width * scale * 0.5f;
            DrawTexturePro(tex,
                { 0, 0, static_cast<float>(tex.width), static_cast<float>(tex.height) },
                { p.x, p.y, tex.width * scale, tex.height * scale },
                { origin, origin },
                p.rotation,
                tint);
        } else {
            // 回退: 短形色块
            float halfSize = TILE_SIZE * 0.4f;
            DrawRectanglePro(
                { p.x, p.y, halfSize * 2, halfSize * 2 },
                { halfSize, halfSize },
                p.rotation,
                teamColor);
        }

        // HP 条
        if (p.maxHp > 1) {
            int barW = 40;
            int barX = static_cast<int>(p.x) - barW / 2;
            int barY = static_cast<int>(p.y) - TILE_SIZE / 2 - 8;
            DrawRectangle(barX, barY, barW, 4, DARKGRAY);
            DrawRectangle(barX, barY, barW * p.hp / p.maxHp, 4, GREEN);
        }

        // 槽位标签
        char label[16];
        if (i == mySlot_)
            snprintf(label, sizeof(label), "P%d*", i + 1);
        else
            snprintf(label, sizeof(label), "P%d", i + 1);
        Color labelColor = (i == mySlot_) ? YELLOW : WHITE;
        DrawText(label, static_cast<int>(p.x) - 8,
                 static_cast<int>(p.y) + TILE_SIZE / 2 + 2, 10, labelColor);
    }
}

void LANGameScene::RenderBullets() {
    static const Color teamColors[] = { RED, BLUE, GREEN, PURPLE };
    for (auto& b : bullets_) {
        if (!b.active) continue;
        Color c = (b.ownerTeam >= 0 && b.ownerTeam < 4) ? teamColors[b.ownerTeam] : WHITE;
        DrawCircle(static_cast<int>(b.x), static_cast<int>(b.y), 4.f, c);
    }
}

void LANGameScene::RenderHUD() {
    char buf[64];

    if (mode_ == GameMode::FREE_FOR_ALL) {
        // FFA: 每个玩家独立显示
        static Color ffaColors[] = { RED, BLUE, GREEN, PURPLE };
        for (int i = 0; i < 4; i++) {
            const char* status = players_[i].alive ? "ALIVE" : "DEAD";
            snprintf(buf, sizeof(buf), "P%d: %s", i + 1, status);
            DrawText(buf, 10, 10 + i * 20, 14, ffaColors[i]);
        }
    } else {
        // 传统/攻防: 队伍统计
        int alive0 = 0, alive1 = 0;
        for (int i = 0; i < 4; i++) {
            if (players_[i].alive) {
                if (players_[i].team == 0) alive0++;
                else alive1++;
            }
        }
        const char* label0, *label1;
        if (mode_ == GameMode::ATTACK_DEFEND) {
            label0 = "ATK"; label1 = "DEF";
        } else {
            label0 = "RED"; label1 = "BLUE";
        }
        snprintf(buf, sizeof(buf), "%s: %d", label0, alive0);
        DrawText(buf, 10, 10, 18, RED);
        snprintf(buf, sizeof(buf), "%s: %d", label1, alive1);
        DrawText(buf, GetScreenWidth() - 80, 10, 18, BLUE);
    }

    // 连接状态 + 自己的槽位
    if (mySlot_ >= 0) {
        snprintf(buf, sizeof(buf), "CLIENT (P%d)", mySlot_ + 1);
    } else {
        snprintf(buf, sizeof(buf), "CLIENT");
    }
    DrawText(buf, 10, GetScreenHeight() - 20, 12, GRAY);
}
