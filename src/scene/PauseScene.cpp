#include "PauseScene.h"
#include "SceneManager.h"
#include "raylib.h"

void PauseScene::Update(float dt) {
    timer_ += dt;

    // 防误触：暂停后 0.3 秒内不接受输入
    if (timer_ < 0.3f) return;

    // P / ESC → 恢复游戏
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
        manager_->PopScene();
    }
    // Q → 退出到主菜单 (清理整个游戏栈)
    if (IsKeyPressed(KEY_Q)) {
        manager_->ReturnToMenu();
    }
}

void PauseScene::Render() {
    // 半透明遮罩
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.6f));

    // 暂停文字
    const char* text = "PAUSED";
    int fontSize = 50;
    int textW = MeasureText(text, fontSize);
    DrawText(text, (GetScreenWidth() - textW) / 2, 330, fontSize, WHITE);

    const char* hint1 = "P / ESC = Resume";
    int hint1W = MeasureText(hint1, 18);
    DrawText(hint1, (GetScreenWidth() - hint1W) / 2, 410, 18, GRAY);

    const char* hint2 = "Q = Quit to Menu";
    int hint2W = MeasureText(hint2, 18);
    DrawText(hint2, (GetScreenWidth() - hint2W) / 2, 440, 18, ORANGE);
}
