// winsock2.h 必须在 windows.h (raylib) 之前包含
#include "net/NetworkManager.h"

#include "LANModeSelectScene.h"
#include "SceneManager.h"
#include "LANLobbyScene.h"
#include "raylib.h"
#include <string>
#include <memory>

LANModeSelectScene::LANModeSelectScene(SceneManager* manager)
    : manager_(manager) {}

void LANModeSelectScene::Enter() {
    selection_ = 0;
}

void LANModeSelectScene::Update(float dt) {
    if (IsKeyPressed(KEY_UP))   selection_ = (selection_ + 2) % 3;
    if (IsKeyPressed(KEY_DOWN)) selection_ = (selection_ + 1) % 3;

    if (IsKeyPressed(KEY_ENTER)) {
        if (selection_ == 0) {
            // Host Game: 创建大厅 (Host 模式)
            auto net = std::make_unique<NetworkManager>();
            manager_->PushScene(std::make_unique<LANLobbyScene>(manager_, std::move(net), true));
        } else if (selection_ == 1) {
            // Join Game: 创建大厅 (Client 模式)
            auto net = std::make_unique<NetworkManager>();
            manager_->PushScene(std::make_unique<LANLobbyScene>(manager_, std::move(net), false));
        } else {
            // Back to Menu
            manager_->PostPopScene();
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->PostPopScene();
    }
}

void LANModeSelectScene::Render() {
    ClearBackground(BLACK);

    const char* title = "LAN MULTIPLAYER";
    int tw = MeasureText(title, 36);
    DrawText(title, (GetScreenWidth() - tw) / 2, 40, 36, GREEN);

    const char* options[] = { "HOST GAME", "JOIN GAME", "BACK" };
    const char* descs[] = {
        "Create a lobby for others to join",
        "Connect to a host on your network",
        "Return to main menu"
    };

    for (int i = 0; i < 3; i++) {
        int y = 180 + i * 120;
        Color color = (i == selection_) ? YELLOW : WHITE;

        if (i == selection_) {
            DrawRectangle(160, y - 5, 500, 80, ColorAlpha(YELLOW, 0.08f));
            DrawRectangleLines(160, y - 5, 500, 80, YELLOW);
        } else {
            DrawRectangleLines(160, y - 5, 500, 80, DARKGRAY);
        }

        int ow = MeasureText(options[i], 28);
        DrawText(options[i], (GetScreenWidth() - ow) / 2, y + 5, 28, color);

        int dw = MeasureText(descs[i], 14);
        DrawText(descs[i], (GetScreenWidth() - dw) / 2, y + 40, 14, GRAY);
    }

    const char* hint = "UP/DOWN Select  |  ENTER Confirm  |  ESC Back";
    int hw = MeasureText(hint, 16);
    DrawText(hint, (GetScreenWidth() - hw) / 2, 700, 16, YELLOW);
}
