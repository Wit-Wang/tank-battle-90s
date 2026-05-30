#include "MenuScene.h"
#include "SceneManager.h"
#include "ModeSelectScene.h"
#include "LANModeSelectScene.h"
#include "raylib.h"

void MenuScene::Enter() {
    titleBlink_ = 0.f;
    modeSelecting_ = false;
    modeSelection_ = 0;
}

void MenuScene::Update(float dt) {
    titleBlink_ += dt;

    if (!modeSelecting_) {
        // 主菜单: 按 ENTER 进入模式选择
        if (IsKeyPressed(KEY_ENTER)) {
            modeSelecting_ = true;
            modeSelection_ = 0;
        }
    } else {
        // 模式选择子菜单
        if (IsKeyPressed(KEY_UP))   modeSelection_ = (modeSelection_ + 2) % 3;
        if (IsKeyPressed(KEY_DOWN)) modeSelection_ = (modeSelection_ + 1) % 3;

        if (IsKeyPressed(KEY_ENTER)) {
            if (modeSelection_ == 0) {
                // Hot Seat (本地热座) → 游戏模式选择
                manager_->PushScene(std::make_unique<ModeSelectScene>(manager_));
            } else if (modeSelection_ == 1) {
                // LAN Multiplayer
                manager_->PushScene(std::make_unique<LANModeSelectScene>(manager_));
            } else {
                // Back
                modeSelecting_ = false;
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            modeSelecting_ = false;
        }
    }
}

void MenuScene::Render() {
    ClearBackground(BLACK);

    // 标题
    const char* title = "TANK BATTLE";
    int titleSize = 60;
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (GetScreenWidth() - titleW) / 2, 200, titleSize, GREEN);

    // 副标题
    const char* sub = "90s EDITION";
    int subSize = 30;
    int subW = MeasureText(sub, subSize);
    DrawText(sub, (GetScreenWidth() - subW) / 2, 280, subSize, DARKGREEN);

    if (!modeSelecting_) {
        // 闪烁提示
        if (std::sin(titleBlink_ * 3.f) > 0.f) {
            const char* prompt = "PRESS ENTER TO START";
            int promptSize = 20;
            int promptW = MeasureText(prompt, promptSize);
            DrawText(prompt, (GetScreenWidth() - promptW) / 2, 500, promptSize, WHITE);
        }
    } else {
        // 模式选择
        const char* modes[] = { "HOT SEAT (Local)", "LAN MULTIPLAYER", "BACK" };
        const char* descs[] = {
            "Play with friends on one device",
            "Play over local network",
            "Return to title"
        };

        for (int i = 0; i < 3; i++) {
            int y = 420 + i * 80;
            Color color = (i == modeSelection_) ? YELLOW : WHITE;

            if (i == modeSelection_) {
                DrawRectangle(200, y - 5, 430, 65, ColorAlpha(YELLOW, 0.08f));
                DrawRectangleLines(200, y - 5, 430, 65, YELLOW);
            } else {
                DrawRectangleLines(200, y - 5, 430, 65, DARKGRAY);
            }

            int mw = MeasureText(modes[i], 24);
            DrawText(modes[i], (GetScreenWidth() - mw) / 2, y + 5, 24, color);

            int dw = MeasureText(descs[i], 14);
            DrawText(descs[i], (GetScreenWidth() - dw) / 2, y + 35, 14, GRAY);
        }
    }

    // 版权
    const char* credit = "A Tribute to Battle City";
    int creditSize = 14;
    int creditW = MeasureText(credit, creditSize);
    DrawText(credit, (GetScreenWidth() - creditW) / 2, 750, creditSize, GRAY);
}
