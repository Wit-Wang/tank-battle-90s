#pragma once

#include "Scene.h"
#include "game/session/GameSession.h"
#include "game/map/MapManager.h"
#include "net/NetProtocol.h"
#include <memory>
#include <string>

class SceneManager;
class NetworkManager;

/// 局域网大厅: Host 选地图/队伍, Client 选坦克类型
///
/// 所有权模型:
///   构造时获得 unique_ptr<NetworkManager> 的所有权。
///   启动游戏前调用 ReleaseNetwork() 将所有权转移给 GameScene / LANGameScene,
///   之后析构时不再关闭网络连接。
class LANLobbyScene : public Scene {
public:
    LANLobbyScene(SceneManager* manager, std::unique_ptr<NetworkManager> net, bool isHost);
    ~LANLobbyScene() override;

    void Enter() override;
    void Update(float dt) override;
    void Render() override;

    /// 将 NetworkManager 所有权转出 (启动游戏前调用)
    NetworkManager* ReleaseNetwork() { return net_.release(); }

private:
    // ---- Host 逻辑 ----
    void HostUpdate(float dt);
    void BroadcastLobbyState();
    void HandleClientMessage(int clientSocket, const NetMessage& msg);

    // ---- Client 逻辑 ----
    void ClientUpdate(float dt);
    void HandleHostMessage(const NetMessage& msg);

    // ---- 渲染 ----
    void RenderHost();
    void RenderClient();
    void RenderSlot(int index, int y, bool selected, bool isHostView);

    SceneManager* manager_;
    std::unique_ptr<NetworkManager> net_;
    bool isHost_;
    GameSession session_;

    // Host 状态
    int currentSlot_ = 0;
    int mapIndex_ = 0;
    int mapCount_ = 1;
    MapManager mapManager_;  // 保存扫描结果，避免重复扫描
    std::string localIP_;

    // Client 状态
    bool connected_ = false;
    bool joining_ = false;
    int mySlot_ = -1;  // JoinAccepted 分配的槽位
    std::string connectStatus_;
    std::string hostIP_;
    int inputField_ = 0;  // 0-14: IP 输入字符位置

    // 连接计时
    float connectTimer_ = 0.f;
    float lobbyTimer_ = 0.f;
};
