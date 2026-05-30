#include "GameOverScene.h"
#include "SceneManager.h"
#include "MapSelectScene.h"
#include "raylib.h"
#include <cstdio>

GameOverScene::GameOverScene(SceneManager* manager, int winningTeam, GameSession session)
    : manager_(manager), session_(std::move(session)),
      winningTeam_(winningTeam), mode_(session_.GetGameMode()) {}

void GameOverScene::Update(float dt) {
    timer_ += dt;

    if (timer_ > 1.f) {
        inputReady_ = true;
    }

    if (!inputReady_) return;

    // ENTER → 再来一局：直接跳到选图界面
    if (IsKeyPressed(KEY_ENTER)) {
        session_.RandomizeAITypes();  // 重新随机 AI 类型
        manager_->SetupNext(
            std::make_unique<MapSelectScene>(manager_, session_));
    }
    // ESC → 回主菜单
    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->ReturnToMenu();
    }
}

void GameOverScene::Render() {
    ClearBackground(BLACK);

    int sw = GetScreenWidth();
    const char* title = nullptr;
    Color titleColor = WHITE;

    if (mode_ == GameMode::FREE_FOR_ALL) {
        // FFA: 显示获胜玩家
        static const char* ffaColors[] = { "RED", "BLUE", "GREEN", "PURPLE" };
        static Color ffaColorVals[] = { RED, BLUE, GREEN, PURPLE };
        if (winningTeam_ >= 0 && winningTeam_ < 4) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s PLAYER WINS!", ffaColors[winningTeam_]);
            // 静态 buffer 用于 DrawText
            static char titleBuf[64];
            snprintf(titleBuf, sizeof(titleBuf), "%s", buf);
            title = titleBuf;
            titleColor = ffaColorVals[winningTeam_];
        } else {
            title = "DRAW!";
            titleColor = WHITE;
        }
    } else if (mode_ == GameMode::ATTACK_DEFEND) {
        if (winningTeam_ == 0) {
            title = "ATTACK WINS!";
            titleColor = RED;
        } else {
            title = "DEFEND WINS!";
            titleColor = BLUE;
        }
    } else {
        // Traditional
        if (winningTeam_ == 0) {
            title = "RED TEAM WINS!";
            titleColor = RED;
        } else {
            title = "BLUE TEAM WINS!";
            titleColor = BLUE;
        }
    }

    int titleSize = 50;
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, 250, titleSize, titleColor);

    // 模式标签
    const char* modeNames[] = { "Traditional", "Attack/Defend", "Free for All" };
    const char* modeName = modeNames[static_cast<int>(mode_)];
    int modeW = MeasureText(modeName, 20);
    DrawText(modeName, (sw - modeW) / 2, 320, 20, GRAY);

    // 提示
    if (inputReady_) {
        const char* hint1 = "ENTER = Play Again   ESC = Main Menu";
        int hint1W = MeasureText(hint1, 18);
        DrawText(hint1, (sw - hint1W) / 2, 400, 18, GRAY);
    }
}
