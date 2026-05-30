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
    currentSlot_ = 0;
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
    // 接受新客户端
    int newSock = net_->AcceptClient();
    if (newSock >= 0) {
        // 分配槽位 (跳过已有客户端的槽位, 最多 4 人)
        int slot = -1;
        bool used[4] = {};
        for (auto& c : net_->GetClients()) {
            if (c.slotIndex >= 0 && c.slotIndex < 4)
                used[c.slotIndex] = true;
        }
        for (int i = 0; i < 4; i++) {
            if (!session_.GetSlot(i).isHuman || i == 0) {
                // 找一个 AI 槽位给客户端 (slot 0 保留给 host)
                if (i > 0 && !used[i]) { slot = i; break; }
            }
        }
        if (slot < 0) {
            // 所有槽位已满, 踢出
            NetMessage kick(NetMessageType::Kick);
            kick.WriteString("Server full");
            net_->SendTo(newSock, kick);
            net_->DisconnectClient(newSock);
        } else {
            // 分配槽位
            auto& clients = net_->GetClients();
            clients.back().slotIndex = slot;
            session_.GetSlot(slot).isHuman = true;

            // 发送接受消息
            NetMessage accepted(NetMessageType::JoinAccepted);
            accepted.WritePayload(static_cast<uint8_t>(slot));
            net_->SendTo(newSock, accepted);

            // 广播新玩家加入
            NetMessage joined(NetMessageType::PlayerJoined);
            joined.WritePayload(static_cast<uint8_t>(slot));
            joined.WriteString("Player");
            net_->BroadcastTCP(joined);

            // 发送当前大厅状态
            BroadcastLobbyState();
        }
    }

    // 处理客户端消息 (逐个检查)
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

    // ENTER: 开始游戏
    if (IsKeyPressed(KEY_ENTER)) {
        // 设置地图路径
        if (mapIndex_ >= 0 && mapIndex_ < mapManager_.GetCount()) {
            session_.SetMapPath(mapManager_.GetMapInfo(mapIndex_).filePath);
        } else {
            session_.SetMapPath("assets/maps/classic.txt");
        }

        // 随机分配 AI 坦克类型 (在发送 GameStart 之前，确保客户端收到随机结果)
        session_.RandomizeAITypes();

        // 通知所有客户端游戏开始 (包含完整会话数据)
        NetMessage start(NetMessageType::GameStart);
        start.WriteString(session_.GetMapPath());
        // 写入 4 个槽位配置
        for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
            const auto& slot = session_.GetSlot(i);
            start.WritePayload(static_cast<uint8_t>(slot.isHuman ? 1 : 0));
            start.WritePayload(static_cast<uint8_t>(static_cast<int>(slot.tankType)));
            start.WritePayload(static_cast<uint8_t>(slot.team));
        }
        net_->BroadcastTCP(start);

        // 转移 NetworkManager 所有权给 GameScene
        auto game = std::make_unique<GameScene>(manager_, std::move(session_));
        game->SetNetworkManager(ReleaseNetwork());

        // 栈: [Menu, LANModeSelect, LANLobby] → [Menu, Game]
        manager_->ReturnToMenu();
        manager_->PushScene(std::move(game));
    }

    // ESC: 退出 (回到主菜单)
    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->ReturnToMenu();
    }
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
            }
            BroadcastLobbyState();
            break;
        }
        case NetMessageType::Ping: {
            NetMessage pong(NetMessageType::Pong);
            net_->SendTo(clientSocket, pong);
            break;
        }
        default: break;
    }
}

void LANLobbyScene::BroadcastLobbyState() {
    NetMessage state(NetMessageType::LobbyState);

    // 写入地图索引
    state.WritePayload(static_cast<uint16_t>(mapIndex_));

    // 写入 4 个槽位
    for (int i = 0; i < 4; i++) {
        const auto& slot = session_.GetSlot(i);
        state.WritePayload(static_cast<uint8_t>(slot.isHuman ? 1 : 0));
        state.WritePayload(static_cast<uint8_t>(static_cast<int>(slot.tankType)));
        state.WritePayload(static_cast<uint8_t>(slot.team));
    }

    net_->BroadcastTCP(state);
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

        // 简易 IP 输入 (数字和点)
        for (int key = KEY_ZERO; key <= KEY_NINE; key++) {
            if (IsKeyPressed(key) && hostIP_.size() < 15) {
                hostIP_ += static_cast<char>('0' + (key - KEY_ZERO));
            }
        }
        if (IsKeyPressed(KEY_PERIOD) && hostIP_.size() < 15) {
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
            manager_->PopScene();
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

    // ESC: 断开并返回
    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->ReturnToMenu();
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
            }
            break;
        }
        case NetMessageType::GameStart: {
            // 解析完整会话数据: 地图路径 + 4 个槽位配置
            std::size_t off = 0;
            std::string mapPath = msg.ReadString(off);
            off += 2 + mapPath.size();  // 2 bytes for uint16_t string length prefix
            session_.SetMapPath(mapPath);
            for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
                auto& slot = session_.GetSlot(i);
                slot.isHuman  = msg.ReadPayload<uint8_t>(off) != 0;  off++;
                slot.tankType = static_cast<TankType>(msg.ReadPayload<uint8_t>(off)); off++;
                slot.team     = msg.ReadPayload<uint8_t>(off);       off++;
            }
            // 转移 NetworkManager 所有权给 LANGameScene
            auto lanGame = std::make_unique<LANGameScene>(manager_, ReleaseNetwork(), session_, mySlot_);
            // 栈: [Menu, LANLobby] → [Menu, LANGame]
            manager_->ReturnToMenu();
            manager_->PushScene(std::move(lanGame));
            break;
        }
        case NetMessageType::Kick: {
            connectStatus_ = "Kicked: " + msg.ReadString();
            net_->Disconnect();
            connected_ = false;
            break;
        }
        case NetMessageType::Pong: {
            // 心跳回复
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

    // 操作提示
    const char* hint = "UP/DOWN=Slot  LEFT/RIGHT=Team  Q/E=Tank  A/D=Map  ENTER=Start  ESC=Cancel";
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

    // 操作提示
    const char* hint = "UP/DOWN=Browse  LEFT/RIGHT=Your Tank  ESC=Disconnect";
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

    // AI 锁定标记 / "YOU" 标记
    if (!slot.isHuman && !isHostView) {
        DrawText("LOCKED", boxX + 480, y + 12, 14, GRAY);
    }
    if (!isHostView && index == mySlot_) {
        DrawText("YOU", boxX + 480, y + 12, 16, YELLOW);
    }
}
