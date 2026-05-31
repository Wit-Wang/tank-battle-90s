// winsock2.h 必须在 windows.h (raylib) 之前包含
#include "net/NetworkManager.h"
#include "server/ServerConfig.h"

#include "ServerLobbyScene.h"
#include "SceneManager.h"
#include "LANGameScene.h"
#include "game/tank/TankType.h"
#include "core/Types.h"
#include "raylib.h"
#include <cstring>

ServerLobbyScene::ServerLobbyScene(SceneManager* manager)
    : manager_(manager) {}

ServerLobbyScene::~ServerLobbyScene() {
    if (net_) {
        net_->Disconnect();
        net_->CloseUDP();
    }
}

void ServerLobbyScene::Enter() {
    timer_ = 0.f;
    state_ = State::CONNECTING;
    statusMessage_ = "Connecting to " + std::string(DEFAULT_SERVER_ADDRESS) + "...";
    mySlot_ = -1;
    currentSlot_ = 0;

    net_ = std::make_unique<NetworkManager>();

    // Default session: Traditional mode, 4 slots (all AI until server assigns)
    session_.SetGameMode(GameMode::TRADITIONAL);

    // Attempt connection
    if (net_->Connect(DEFAULT_SERVER_ADDRESS, SERVER_PORT)) {
        // Send join request
        NetMessage join(NetMessageType::JoinRequest);
        net_->Send(join);
        statusMessage_ = "Connected! Waiting for server...";
        state_ = State::LOBBY;
    } else {
        statusMessage_ = "Failed to connect. Press ENTER to retry, ESC to go back.";
        state_ = State::ERROR;
    }
}

void ServerLobbyScene::Update(float dt) {
    timer_ += dt;

    switch (state_) {
        case State::CONNECTING:
            // Should not reach here (Enter handles connection)
            break;

        case State::ERROR:
            if (IsKeyPressed(KEY_ENTER)) {
                // Retry
                Enter();
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                manager_->PostReturnToMenu();
            }
            break;

        case State::LOBBY: {
            // Receive messages from server
            NetMessage msg;
            while (net_->Receive(msg)) {
                HandleServerMessage(msg);
            }

            // LEFT/RIGHT: change own tank type
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
                if (mySlot_ >= 0 && mySlot_ < 4) {
                    auto& slot = session_.GetSlot(mySlot_);
                    int t = static_cast<int>(slot.tankType);
                    if (IsKeyPressed(KEY_LEFT))  t = (t + 3) % 4;
                    if (IsKeyPressed(KEY_RIGHT)) t = (t + 1) % 4;
                    slot.tankType = static_cast<TankType>(t);

                    NetMessage setType(NetMessageType::SetTankType);
                    setType.WritePayload(static_cast<uint8_t>(t));
                    net_->Send(setType);
                }
            }

            // UP/DOWN: browse slots
            if (IsKeyPressed(KEY_UP))   currentSlot_ = (currentSlot_ + 3) % 4;
            if (IsKeyPressed(KEY_DOWN)) currentSlot_ = (currentSlot_ + 1) % 4;

            // ESC: disconnect and return
            if (IsKeyPressed(KEY_ESCAPE)) {
                manager_->PostReturnToMenu();
            }
            break;
        }
    }
}

void ServerLobbyScene::HandleServerMessage(const NetMessage& msg) {
    switch (msg.header.type) {
        case NetMessageType::JoinAccepted: {
            uint8_t slot = msg.ReadPayload<uint8_t>();
            mySlot_ = slot;
            statusMessage_ = "Joined as Player " + std::to_string(slot + 1);
            break;
        }
        case NetMessageType::JoinRejected: {
            statusMessage_ = "Rejected: " + msg.ReadString();
            net_->Disconnect();
            state_ = State::ERROR;
            break;
        }
        case NetMessageType::LobbyState: {
            std::size_t offset = 0;
            mapIndex_ = msg.ReadPayload<uint16_t>(offset);
            offset += 2;
            for (int i = 0; i < 4; i++) {
                auto& slot = session_.GetSlot(i);
                slot.isHuman  = msg.ReadPayload<uint8_t>(offset) != 0;  offset++;
                slot.tankType = static_cast<TankType>(msg.ReadPayload<uint8_t>(offset)); offset++;
                slot.team     = msg.ReadPayload<uint8_t>(offset);       offset++;
            }
            break;
        }
        case NetMessageType::GameStart: {
            // Parse full session data: map path + 4 slot configs
            std::size_t off = 0;
            std::string mapPath = msg.ReadString(off);
            off += 2 + mapPath.size();
            session_.SetMapPath(mapPath);
            for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
                auto& slot = session_.GetSlot(i);
                slot.isHuman  = msg.ReadPayload<uint8_t>(off) != 0;  off++;
                slot.tankType = static_cast<TankType>(msg.ReadPayload<uint8_t>(off)); off++;
                slot.team     = msg.ReadPayload<uint8_t>(off);       off++;
            }

            // Deferred: transfer network ownership and switch to game scene
            auto capturedSession = session_;
            int capturedSlot = mySlot_;
            auto* mgr = manager_;
            manager_->PostAction([this, mgr, capturedSession, capturedSlot]() mutable {
                TraceLog(LOG_INFO, "SERVER CLIENT: starting game");
                auto lanGame = std::make_unique<LANGameScene>(
                    mgr, ReleaseNetwork(), std::move(capturedSession), capturedSlot);
                mgr->ReturnToMenu();
                mgr->PushScene(std::move(lanGame));
            });
            break;
        }
        case NetMessageType::Kick: {
            statusMessage_ = "Kicked: " + msg.ReadString();
            net_->Disconnect();
            state_ = State::ERROR;
            break;
        }
        case NetMessageType::Pong:
            break;
        default:
            break;
    }
}

void ServerLobbyScene::Render() {
    ClearBackground(BLACK);

    switch (state_) {
        case State::CONNECTING:
        case State::ERROR:
            RenderConnecting();
            break;
        case State::LOBBY:
            RenderLobby();
            break;
    }
}

void ServerLobbyScene::RenderConnecting() {
    int sw = GetScreenWidth();

    const char* title = "SERVER MULTIPLAYER";
    DrawText(title, (sw - MeasureText(title, 32)) / 2, 15, 32, GREEN);

    std::string addr = "Server: " + std::string(DEFAULT_SERVER_ADDRESS)
                       + ":" + std::to_string(SERVER_PORT);
    DrawText(addr.c_str(), (sw - MeasureText(addr.c_str(), 18)) / 2, 60, 18, YELLOW);

    DrawText(statusMessage_.c_str(),
             (sw - MeasureText(statusMessage_.c_str(), 18)) / 2, 300, 18, ORANGE);

    if (state_ == State::ERROR) {
        const char* hint = "ENTER = Retry  |  ESC = Back";
        DrawText(hint, (sw - MeasureText(hint, 16)) / 2, 700, 16, YELLOW);
    }
}

void ServerLobbyScene::RenderLobby() {
    int sw = GetScreenWidth();

    const char* title = "SERVER LOBBY";
    DrawText(title, (sw - MeasureText(title, 32)) / 2, 15, 32, GREEN);

    std::string addr = "Server: " + std::string(DEFAULT_SERVER_ADDRESS);
    DrawText(addr.c_str(), (sw - MeasureText(addr.c_str(), 16)) / 2, 55, 16, YELLOW);

    DrawText(statusMessage_.c_str(),
             (sw - MeasureText(statusMessage_.c_str(), 16)) / 2, 80, 16, WHITE);

    // Slots
    for (int i = 0; i < 4; i++) {
        RenderSlot(i, 130 + i * 90, i == currentSlot_);
    }

    // Controls
    const char* hint = "UP/DOWN=Browse  LEFT/RIGHT=Your Tank  ESC=Disconnect";
    DrawText(hint, (sw - MeasureText(hint, 16)) / 2, 700, 16, YELLOW);
}

void ServerLobbyScene::RenderSlot(int index, int y, bool selected) {
    int sw = GetScreenWidth();
    int boxW = 600;
    int boxX = (sw - boxW) / 2;

    const auto& slot = session_.GetSlot(index);
    Color highlight = selected ? YELLOW : DARKGRAY;
    Color teamColor = (slot.team == 0) ? RED : BLUE;
    if (slot.team >= 0 && slot.team < 4) {
        static const Color teamColors[] = { RED, BLUE, GREEN, PURPLE };
        teamColor = teamColors[slot.team];
    }

    if (selected) {
        DrawRectangle(boxX, y, boxW, 70, ColorAlpha(YELLOW, 0.08f));
    }
    DrawRectangleLines(boxX, y, boxW, 70, highlight);

    // Player number
    char buf[64];
    snprintf(buf, sizeof(buf), "P%d", index + 1);
    DrawText(buf, boxX + 15, y + 8, 26, highlight);

    // HUMAN / AI
    const char* typeStr = slot.isHuman ? "HUMAN" : "AI";
    Color typeColor = slot.isHuman ? GREEN : ORANGE;
    DrawText(typeStr, boxX + 70, y + 12, 18, typeColor);

    // Tank type (editable if own slot)
    const auto& stats = GetTankStats(slot.tankType);
    if (index == mySlot_) {
        DrawText("< ", boxX + 170, y + 8, 22, YELLOW);
        DrawText(stats.name, boxX + 195, y + 8, 22, WHITE);
        DrawText(" >", boxX + 195 + MeasureText(stats.name, 22) + 8, y + 8, 22, YELLOW);
    } else {
        DrawText(stats.name, boxX + 170, y + 8, 22, LIGHTGRAY);
    }

    // Team
    const char* teamStr = (slot.team == 0) ? "RED" : "BLUE";
    if (slot.team == 2) teamStr = "GREEN";
    if (slot.team == 3) teamStr = "PURPLE";
    DrawText(teamStr, boxX + 420, y + 12, 20, teamColor);

    // YOU marker
    if (index == mySlot_) {
        DrawText("YOU", boxX + 480, y + 12, 16, YELLOW);
    } else if (!slot.isHuman) {
        DrawText("AI", boxX + 480, y + 12, 14, GRAY);
    }
}
