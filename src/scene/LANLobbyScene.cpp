// winsock2.h 必须在 windows.h (raylib) 之前包含
#include "net/NetworkManager.h"

#include "LANLobbyScene.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "LANGameScene.h"
#include "game/map/MapManager.h"
#include "game/tank/TankType.h"
#include "raylib.h"
#include <cstring>

LANLobbyScene::LANLobbyScene(SceneManager* manager,
                               std::unique_ptr<NetworkManager> net, bool isHost)
    : manager_(manager), net_(std::move(net)), isHost_(isHost) {}

LANLobbyScene::~LANLobbyScene() {
    // 如果 net_ 还在 (未通过 ReleaseNetwork() 转移), 说明不是启动游戏而是退出
    if (net_) {
        if (isHost_) {
            net_->DisconnectAllClients();
            net_->Close();
        } else {
            net_->Disconnect();
        }
        net_->CloseUDP();
    }
}

void LANLobbyScene::Enter() {
    lobbyTimer_ = 0.f;
    heartbeatTimer_ = 0.f;
    currentSlot_ = 0;
    hostReady_ = false;
    session_.SetGameMode(GameMode::TRADITIONAL);  // LAN 默认传统模式

    if (isHost_) {
        // LAN 主机只有 1 个本地玩家 (slot 0);
        // Traditional 预设把 slot 1 也标为 human (热座用)，需要重置
        for (int i = 1; i < GameSession::SLOT_COUNT; i++) {
            session_.GetSlot(i).isHuman = false;
        }

        // Host: 开始监听
        if (net_->Listen(NET_DEFAULT_PORT)) {
            localIP_ = net_->GetLocalIP();
        } else {
            localIP_ = "LISTEN FAILED";
        }
        // 扫描地图 (仅传统模式)
        mapManager_.ScanMapsForMode("assets/maps", GameMode::TRADITIONAL);
        mapCount_ = mapManager_.GetCount();
        mapIndex_ = 0;
    } else {
        // Client: 准备连接
        joining_ = false;
        connected_ = false;
        connectStatus_ = "Enter host IP and press ENTER";
        hostIP_ = "127.0.0.1";
        connectTimer_ = 0.f;
    }
}

void LANLobbyScene::Update(float dt) {
    lobbyTimer_ += dt;
    if (isHost_) HostUpdate(dt);
    else         ClientUpdate(dt);
}

// ============================================================
//  Host Update
// ============================================================
void LANLobbyScene::HostUpdate(float dt) {
    // 心跳: 定期向客户端发 Ping, 检测超时
    heartbeatTimer_ += dt;
    if (heartbeatTimer_ >= NET_PING_INTERVAL) {
        heartbeatTimer_ = 0.f;
        NetMessage ping(NetMessageType::Ping);
        net_->BroadcastTCP(ping);
        // 检查超时的客户端
        auto& clients = net_->GetClients();
        for (int i = static_cast<int>(clients.size()) - 1; i >= 0; i--) {
            if (clients[i].lastPing > 0.f &&
                lobbyTimer_ - clients[i].lastPing > NET_TIMEOUT_SECONDS) {
                printf("[HOST] Client P%d timed out\n", clients[i].slotIndex + 1);
                int slot = clients[i].slotIndex;
                session_.GetSlot(slot).isHuman = false;
                net_->DisconnectClient(clients[i].socket);
                BroadcastLobbyState();
            }
        }
    }

    // 接受新客户端
    int newSock = net_->AcceptClient();
    if (newSock >= 0) {
        // 分配槽位: 找第一个空闲的 AI 槽位 (slot 0 保留给 host)
        int slot = -1;
        bool used[4] = {};
        for (auto& c : net_->GetClients()) {
            if (c.slotIndex >= 0 && c.slotIndex < 4)
                used[c.slotIndex] = true;
        }
        for (int i = 1; i < 4; i++) {
            if (!used[i]) { slot = i; break; }
        }
        if (slot < 0) {
            NetMessage kick(NetMessageType::Kick);
            kick.WriteString("Server full");
            net_->SendTo(newSock, kick);
            net_->DisconnectClient(newSock);
        } else {
            auto& clients = net_->GetClients();
            clients.back().slotIndex = slot;
            clients.back().lastPing = lobbyTimer_;
            session_.GetSlot(slot).isHuman = true;
            // 新加入的客户端默认未准备
            clients.back().ready = false;

            NetMessage accepted(NetMessageType::JoinAccepted);
            accepted.WritePayload(static_cast<uint8_t>(slot));
            net_->SendTo(newSock, accepted);

            NetMessage joined(NetMessageType::PlayerJoined);
            joined.WritePayload(static_cast<uint8_t>(slot));
            joined.WriteString("Player");
            net_->BroadcastTCP(joined);

            BroadcastLobbyState();
        }
    }

    // 处理客户端消息
    for (auto& client : net_->GetClients()) {
        NetMessage msg;
        while (net_->ReceiveFromClient(client.socket, msg)) {
            HandleClientMessage(client.socket, msg);
        }
    }

    // 上下切换槽位
    if (IsKeyPressed(KEY_UP))   currentSlot_ = (currentSlot_ + 3) % 4;
    if (IsKeyPressed(KEY_DOWN)) currentSlot_ = (currentSlot_ + 1) % 4;

    // 左右: 切换当前槽位的队伍
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
        auto& slot = session_.GetSlot(currentSlot_);
        slot.team = 1 - slot.team;
        session_.BalanceAITeams();
        BroadcastLobbyState();
    }

    // Q/E: 切换当前槽位的坦克类型 (仅人类槽位)
    if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_E)) {
        auto& slot = session_.GetSlot(currentSlot_);
        if (slot.isHuman) {
            int t = static_cast<int>(slot.tankType);
            t = IsKeyPressed(KEY_Q) ? (t + 3) % 4 : (t + 1) % 4;
            slot.tankType = static_cast<TankType>(t);
            BroadcastLobbyState();
        }
    }

    // A/D: 切换地图
    if (IsKeyPressed(KEY_A)) {
        mapIndex_ = (mapIndex_ + mapCount_ - 1) % mapCount_;
        BroadcastLobbyState();
    }
    if (IsKeyPressed(KEY_D)) {
        mapIndex_ = (mapIndex_ + 1) % mapCount_;
        BroadcastLobbyState();
    }

    // ENTER: 准备 / 取消准备 (host 切换自己的 ready 状态)
    if (IsKeyPressed(KEY_ENTER)) {
        hostReady_ = !hostReady_;
        BroadcastLobbyState();
    }

    // 所有人准备好后自动开始游戏
    if (hostReady_ && AllPlayersReady()) {
        // 设置地图路径
        if (mapIndex_ >= 0 && mapIndex_ < mapManager_.GetCount()) {
            session_.SetMapPath(mapManager_.GetMapInfo(mapIndex_).filePath);
        } else {
            session_.SetMapPath("assets/maps/classic.txt");
        }

        session_.RandomizeAITypes();

        NetMessage start(NetMessageType::GameStart);
        start.WriteString(session_.GetMapPath());
        for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
            const auto& slot = session_.GetSlot(i);
            start.WritePayload(static_cast<uint8_t>(slot.isHuman ? 1 : 0));
            start.WritePayload(static_cast<uint8_t>(static_cast<int>(slot.tankType)));
            start.WritePayload(static_cast<uint8_t>(slot.team));
        }
        net_->BroadcastTCP(start);

        auto* mgr = manager_;
        manager_->PostAction([this, mgr]() {
            TraceLog(LOG_INFO, "HOST: starting game, transferring network ownership");
            auto game = std::make_unique<GameScene>(mgr, std::move(session_));
            game->SetNetworkManager(ReleaseNetwork());
            TraceLog(LOG_INFO, "HOST: ReturnToMenu + PushScene(GameScene)");
            mgr->ReturnToMenu();
            mgr->PushScene(std::move(game));
            TraceLog(LOG_INFO, "HOST: game scene pushed successfully");
        });
    }

    // ESC: 退出 (回到主菜单)
    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->PostReturnToMenu();
    }
}

bool LANLobbyScene::AllPlayersReady() const {
    // 检查所有人类槽位是否都已准备
    // host (slot 0) 用 hostReady_
    if (!hostReady_) return false;
    for (auto& client : net_->GetClients()) {
        if (client.slotIndex >= 0 && !client.ready) return false;
    }
    return true;
}

void LANLobbyScene::HandleClientMessage(int clientSocket, const NetMessage& msg) {
    switch (msg.header.type) {
        case NetMessageType::SetTankType: {
            uint8_t tankType = msg.ReadPayload<uint8_t>();
            int ci = net_->FindClientBySocket(clientSocket);
            if (ci >= 0) {
                int slot = net_->GetClients()[ci].slotIndex;
                if (slot >= 0 && slot < 4) {
                    session_.GetSlot(slot).tankType = static_cast<TankType>(tankType);
                }
            }
            BroadcastLobbyState();
            break;
        }
        case NetMessageType::ClientReady: {
            int ci = net_->FindClientBySocket(clientSocket);
            if (ci >= 0) {
                net_->GetClients()[ci].ready = !net_->GetClients()[ci].ready;
                printf("[HOST] Player P%d ready=%d\n",
                       net_->GetClients()[ci].slotIndex + 1,
                       net_->GetClients()[ci].ready ? 1 : 0);
            }
            BroadcastLobbyState();
            break;
        }
        case NetMessageType::Ping: {
            NetMessage pong(NetMessageType::Pong);
            net_->SendTo(clientSocket, pong);
            break;
        }
        case NetMessageType::Pong: {
            // 客户端回复心跳, 更新 lastPing
            int ci = net_->FindClientBySocket(clientSocket);
            if (ci >= 0) {
                net_->GetClients()[ci].lastPing = lobbyTimer_;
            }
            break;
        }
        default: break;
    }
}

void LANLobbyScene::BroadcastLobbyState() {
    NetMessage state(NetMessageType::LobbyState);

    // 写入地图索引
    state.WritePayload(static_cast<uint16_t>(mapIndex_));

    // 写入 4 个槽位 (isHuman, tankType, team, ready)
    for (int i = 0; i < 4; i++) {
        const auto& slot = session_.GetSlot(i);
        state.WritePayload(static_cast<uint8_t>(slot.isHuman ? 1 : 0));
        state.WritePayload(static_cast<uint8_t>(static_cast<int>(slot.tankType)));
        state.WritePayload(static_cast<uint8_t>(slot.team));
        // ready 标志: host (slot 0) 用自己的 hostReady_, 客户端用 ConnectedClient.ready
        bool ready = false;
        if (i == 0) {
            ready = hostReady_;
        } else {
            int ci = FindClientBySlot(i);
            if (ci >= 0) ready = net_->GetClients()[ci].ready;
        }
        state.WritePayload(static_cast<uint8_t>(ready ? 1 : 0));
    }

    net_->BroadcastTCP(state);
}

int LANLobbyScene::FindClientBySlot(int slot) const {
    for (size_t i = 0; i < net_->GetClients().size(); i++) {
        if (net_->GetClients()[i].slotIndex == slot)
            return static_cast<int>(i);
    }
    return -1;
}

// ============================================================
//  Client Update
// ============================================================
void LANLobbyScene::ClientUpdate(float dt) {
    connectTimer_ += dt;

    if (!connected_) {
        // IP 输入
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (!hostIP_.empty()) hostIP_.pop_back();
        }

        // 简易 IP 输入 (主键盘 + 小键盘数字和点)
        for (int key = KEY_ZERO; key <= KEY_NINE; key++) {
            if (IsKeyPressed(key) && hostIP_.size() < 15) {
                hostIP_ += static_cast<char>('0' + (key - KEY_ZERO));
            }
        }
        for (int key = KEY_KP_0; key <= KEY_KP_9; key++) {
            if (IsKeyPressed(key) && hostIP_.size() < 15) {
                hostIP_ += static_cast<char>('0' + (key - KEY_KP_0));
            }
        }
        if ((IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_KP_DECIMAL)) && hostIP_.size() < 15) {
            hostIP_ += '.';
        }

        // ENTER: 连接
        if (IsKeyPressed(KEY_ENTER) && !hostIP_.empty()) {
            connectStatus_ = "Connecting to " + hostIP_ + "...";
            if (net_->Connect(hostIP_, NET_DEFAULT_PORT)) {
                connected_ = true;
                joining_ = true;
                connectStatus_ = "Connected! Waiting for lobby state...";
                // 发送加入请求
                NetMessage join(NetMessageType::JoinRequest);
                net_->Send(join);
            } else {
                connectStatus_ = "Connection failed. Check IP and try again.";
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            manager_->PostPopScene();
        }
        return;
    }

    // 已连接: 接收消息
    NetMessage msg;
    while (net_->Receive(msg)) {
        HandleHostMessage(msg);
    }

    // 上下: 切换浏览槽位
    if (IsKeyPressed(KEY_UP))   currentSlot_ = (currentSlot_ + 3) % 4;
    if (IsKeyPressed(KEY_DOWN)) currentSlot_ = (currentSlot_ + 1) % 4;

    // 左右: 切换自己的坦克类型
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
        if (mySlot_ >= 0 && mySlot_ < 4) {
            auto& slot = session_.GetSlot(mySlot_);
            int t = static_cast<int>(slot.tankType);
            if (IsKeyPressed(KEY_LEFT))  t = (t + 3) % 4;
            if (IsKeyPressed(KEY_RIGHT)) t = (t + 1) % 4;
            slot.tankType = static_cast<TankType>(t);

            // 通知 Host
            NetMessage setType(NetMessageType::SetTankType);
            setType.WritePayload(static_cast<uint8_t>(t));
            net_->Send(setType);
        }
    }

    // ENTER: 准备 / 取消准备
    if (IsKeyPressed(KEY_ENTER) && mySlot_ >= 0) {
        NetMessage ready(NetMessageType::ClientReady);
        net_->Send(ready);
    }

    // ESC: 断开并返回
    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->PostReturnToMenu();
    }
}

void LANLobbyScene::HandleHostMessage(const NetMessage& msg) {
    switch (msg.header.type) {
        case NetMessageType::JoinAccepted: {
            uint8_t slot = msg.ReadPayload<uint8_t>();
            mySlot_ = slot;
            connectStatus_ = "Joined as Player " + std::to_string(slot + 1);
            joining_ = false;
            break;
        }
        case NetMessageType::JoinRejected: {
            connectStatus_ = "Rejected: " + msg.ReadString();
            net_->Disconnect();
            connected_ = false;
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
                slotReady_[i] = msg.ReadPayload<uint8_t>(offset) != 0;  offset++;
            }
            break;
        }
        case NetMessageType::GameStart: {
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
            auto capturedSession = session_;
            int capturedSlot = mySlot_;
            auto* mgr = manager_;
            manager_->PostAction([this, mgr, capturedSession, capturedSlot]() mutable {
                TraceLog(LOG_INFO, "CLIENT: starting game, transferring network ownership");
                auto lanGame = std::make_unique<LANGameScene>(
                    mgr, ReleaseNetwork(), std::move(capturedSession), capturedSlot);
                TraceLog(LOG_INFO, "CLIENT: ReturnToMenu + PushScene(LANGameScene)");
                mgr->ReturnToMenu();
                mgr->PushScene(std::move(lanGame));
                TraceLog(LOG_INFO, "CLIENT: LANGameScene pushed successfully");
            });
            break;
        }
        case NetMessageType::Kick: {
            connectStatus_ = "Kicked: " + msg.ReadString();
            net_->Disconnect();
            connected_ = false;
            break;
        }
        case NetMessageType::Ping: {
            // 心跳: 回复 Pong
            NetMessage pong(NetMessageType::Pong);
            net_->Send(pong);
            break;
        }
        case NetMessageType::Pong: {
            break;
        }
        default: break;
    }
}

// ============================================================
//  Render
// ============================================================
void LANLobbyScene::Render() {
    ClearBackground(BLACK);
    if (isHost_) RenderHost();
    else         RenderClient();
}

void LANLobbyScene::RenderHost() {
    int sw = GetScreenWidth();

    const char* title = "LAN LOBBY (HOST)";
    DrawText(title, (sw - MeasureText(title, 32)) / 2, 15, 32, GREEN);

    // IP 地址
    std::string ipStr = "Your IP: " + localIP_ + "  Port: " + std::to_string(NET_DEFAULT_PORT);
    DrawText(ipStr.c_str(), (sw - MeasureText(ipStr.c_str(), 20)) / 2, 55, 20, YELLOW);

    // 连接的玩家数
    char countBuf[32];
    snprintf(countBuf, sizeof(countBuf), "Players: %d", net_->ClientCount() + 1);
    DrawText(countBuf, (sw - MeasureText(countBuf, 18)) / 2, 80, 18, WHITE);

    // 地图选择
    if (mapIndex_ >= 0 && mapIndex_ < mapManager_.GetCount()) {
        const auto& info = mapManager_.GetMapInfo(mapIndex_);
        std::string mapStr = "< " + info.name + " >";
        DrawText(mapStr.c_str(), (sw - MeasureText(mapStr.c_str(), 24)) / 2, 108, 24, WHITE);
    }
    DrawText("A/D = Change Map", (sw - MeasureText("A/D = Change Map", 14)) / 2, 136, 14, GRAY);

    // 槽位
    for (int i = 0; i < 4; i++) {
        RenderSlot(i, 170 + i * 90, i == currentSlot_, true);
    }

    // 准备状态提示
    const char* readyHint = hostReady_ ? "You are READY (ENTER to cancel)" : "Press ENTER to ready up";
    Color readyHintColor = hostReady_ ? GREEN : YELLOW;
    DrawText(readyHint, (sw - MeasureText(readyHint, 18)) / 2, 730, 18, readyHintColor);

    // 操作提示
    const char* hint = "UP/DOWN=Slot  LEFT/RIGHT=Team  Q/E=Tank  A/D=Map  ENTER=Ready  ESC=Cancel";
    DrawText(hint, (sw - MeasureText(hint, 16)) / 2, 760, 16, YELLOW);
}

void LANLobbyScene::RenderClient() {
    int sw = GetScreenWidth();

    const char* title = "LAN LOBBY (CLIENT)";
    DrawText(title, (sw - MeasureText(title, 32)) / 2, 15, 32, GREEN);

    if (!connected_) {
        // 连接界面
        DrawText("Host IP:", (sw - 200) / 2, 120, 22, WHITE);

        // IP 输入框
        DrawRectangle((sw - 250) / 2, 155, 250, 36, DARKGRAY);
        DrawRectangleLines((sw - 250) / 2, 155, 250, 36, YELLOW);
        DrawText(hostIP_.c_str(), (sw - 250) / 2 + 10, 160, 24, WHITE);

        // 光标闪烁
        if (std::sin(lobbyTimer_ * 5.f) > 0.f) {
            int cw = MeasureText(hostIP_.c_str(), 24);
            DrawText("_", (sw - 250) / 2 + 10 + cw, 160, 24, YELLOW);
        }

        DrawText(connectStatus_.c_str(),
                 (sw - MeasureText(connectStatus_.c_str(), 18)) / 2, 220, 18, ORANGE);

        DrawText("ENTER = Connect  |  ESC = Back",
                 (sw - MeasureText("ENTER = Connect  |  ESC = Back", 16)) / 2, 700, 16, YELLOW);
        return;
    }

    // 已连接: 显示大厅状态
    DrawText(connectStatus_.c_str(),
             (sw - MeasureText(connectStatus_.c_str(), 16)) / 2, 55, 16, YELLOW);

    // 槽位 (只读, 除了自己的坦克类型)
    for (int i = 0; i < 4; i++) {
        RenderSlot(i, 120 + i * 100, i == currentSlot_, false);
    }

    // 准备状态提示
    bool myReady = (mySlot_ >= 0 && mySlot_ < 4) ? slotReady_[mySlot_] : false;
    const char* readyHint = myReady ? "You are READY (ENTER to cancel)" : "Press ENTER to ready up";
    Color readyHintColor = myReady ? GREEN : YELLOW;
    DrawText(readyHint, (sw - MeasureText(readyHint, 18)) / 2, 660, 18, readyHintColor);

    // 操作提示
    const char* hint = "UP/DOWN=Browse  LEFT/RIGHT=Your Tank  ENTER=Ready  ESC=Disconnect";
    DrawText(hint, (sw - MeasureText(hint, 16)) / 2, 700, 16, YELLOW);
}

void LANLobbyScene::RenderSlot(int index, int y, bool selected, bool isHostView) {
    int sw = GetScreenWidth();
    int boxW = 600;
    int boxX = (sw - boxW) / 2;

    const auto& slot = session_.GetSlot(index);
    Color highlight = selected ? YELLOW : DARKGRAY;
    Color teamColor = (slot.team == 0) ? RED : BLUE;

    if (selected) {
        DrawRectangle(boxX, y, boxW, 70, ColorAlpha(YELLOW, 0.08f));
    }
    DrawRectangleLines(boxX, y, boxW, 70, highlight);

    // 编号
    char buf[64];
    snprintf(buf, sizeof(buf), "P%d", index + 1);
    DrawText(buf, boxX + 15, y + 8, 26, highlight);

    // 人/AI
    const char* typeStr = slot.isHuman ? "HUMAN" : "AI";
    Color typeColor = slot.isHuman ? GREEN : ORANGE;
    DrawText(typeStr, boxX + 70, y + 12, 18, typeColor);

    // 坦克类型
    const auto& stats = GetTankStats(slot.tankType);
    if (isHostView || slot.isHuman) {
        DrawText("< ", boxX + 170, y + 8, 22, YELLOW);
        DrawText(stats.name, boxX + 195, y + 8, 22, WHITE);
        DrawText(" >", boxX + 195 + MeasureText(stats.name, 22) + 8, y + 8, 22, YELLOW);
    } else {
        DrawText(stats.name, boxX + 170, y + 8, 22, LIGHTGRAY);
    }

    // 队伍
    const char* teamStr = (slot.team == 0) ? "RED" : "BLUE";
    DrawText(teamStr, boxX + 420, y + 12, 20, teamColor);

    // AI 锁定标记 / "YOU" 标记 / 准备状态
    if (!slot.isHuman && !isHostView) {
        DrawText("LOCKED", boxX + 480, y + 12, 14, GRAY);
    }
    if (!isHostView && index == mySlot_) {
        DrawText("YOU", boxX + 480, y + 12, 16, YELLOW);
    }

    // 准备状态标记
    if (slot.isHuman) {
        bool ready = false;
        if (isHostView) {
            if (index == 0) ready = hostReady_;
            else {
                int ci = FindClientBySlot(index);
                if (ci >= 0) ready = net_->GetClients()[ci].ready;
            }
        } else {
            ready = slotReady_[index];
        }
        const char* readyStr = ready ? "READY" : "NOT READY";
        Color readyColor = ready ? GREEN : RED;
        DrawText(readyStr, boxX + 480, y + 40, 14, readyColor);
    }
}
