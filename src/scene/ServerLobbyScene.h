#pragma once

#include "Scene.h"
#include "game/session/GameSession.h"
#include "net/NetProtocol.h"
#include <memory>
#include <string>

class SceneManager;
class NetworkManager;

/// Server multiplayer lobby: connect to remote server, configure tank/team, wait for game start
class ServerLobbyScene : public Scene {
public:
    explicit ServerLobbyScene(SceneManager* manager);
    ~ServerLobbyScene() override;

    void Enter() override;
    void Update(float dt) override;
    void Render() override;

    NetworkManager* ReleaseNetwork() { return net_.release(); }

private:
    void HandleServerMessage(const NetMessage& msg);
    void RenderConnecting();
    void RenderLobby();
    void RenderSlot(int index, int y, bool selected);

    SceneManager* manager_;
    std::unique_ptr<NetworkManager> net_;
    GameSession session_;

    // Connection state
    enum class State { CONNECTING, LOBBY, ERROR };
    State state_ = State::CONNECTING;
    int mySlot_ = -1;
    std::string statusMessage_;
    float timer_ = 0.f;

    // Lobby state
    int currentSlot_ = 0;
    int mapIndex_ = 0;
    int gameMode_ = 0;  // GameMode enum from server
    bool slotReady_[4] = {};
};
