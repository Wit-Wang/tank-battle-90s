#include "ModeSelectScene.h"
#include "SceneManager.h"
#include "SetupScene.h"
#include "raylib.h"

void ModeSelectScene::Enter() {
    selection_ = 0;
    animTimer_ = 0.f;
}

void ModeSelectScene::Update(float dt) {
    animTimer_ += dt;

    if (IsKeyPressed(KEY_UP))   selection_ = (selection_ + 2) % 3;
    if (IsKeyPressed(KEY_DOWN)) selection_ = (selection_ + 1) % 3;

    if (IsKeyPressed(KEY_ENTER)) {
        GameMode mode = static_cast<GameMode>(selection_);
        manager_->PushScene(std::make_unique<SetupScene>(manager_, mode));
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->PostPopScene();
    }
}

void ModeSelectScene::RenderModeCard(int index, int y) {
    int sw = GetScreenWidth();
    int cardW = 600;
    int cardH = 140;
    int cardX = (sw - cardW) / 2;
    bool selected = (index == selection_);

    // 卡片背景
    Color bgColor = selected ? ColorAlpha(YELLOW, 0.1f) : ColorAlpha(DARKGRAY, 0.15f);
    DrawRectangle(cardX, y, cardW, cardH, bgColor);

    Color borderColor = selected ? YELLOW : DARKGRAY;
    DrawRectangleLines(cardX, y, cardW, cardH, borderColor);

    // 模式信息
    const char* names[] = { "TRADITIONAL", "ATTACK / DEFEND", "FREE FOR ALL" };
    const char* descs[] = {
        "2 teams (2 Humans + 2 AI). Destroy the enemy base or eliminate all enemies.",
        "Attackers vs Defenders (2 Humans + 2 AI). Destroy the base or survive.",
        "No teams (2 Humans + 2 AI). Every player for themselves. Last one standing wins."
    };
    const char* configs[] = {
        "2v2  |  2 Humans + 2 AI  |  No respawn  |  2 bases",
        "2v2  |  2 Humans + 2 AI  |  Defenders respawn (3s)  |  1 base",
        "FFA  |  2 Humans + 2 AI  |  No respawn  |  No bases"
    };
    Color modeColors[] = { GREEN, RED, PURPLE };

    // 模式名称
    int nameSize = 28;
    int nameW = MeasureText(names[index], nameSize);
    DrawText(names[index], cardX + (cardW - nameW) / 2, y + 12, nameSize, modeColors[index]);

    // 描述 (自动换行: 简单方案 — 截断到卡片宽度)
    int descSize = 13;
    int descW = MeasureText(descs[index], descSize);
    if (descW > cardW - 40) {
        // 简单截断
        char truncated[128];
        snprintf(truncated, sizeof(truncated), "%.80s...", descs[index]);
        DrawText(truncated, cardX + 20, y + 50, descSize, LIGHTGRAY);
    } else {
        DrawText(descs[index], cardX + 20, y + 50, descSize, LIGHTGRAY);
    }

    // 配置摘要
    int cfgSize = 12;
    DrawText(configs[index], cardX + 20, y + 75, cfgSize, GRAY);

    // 选中指示器
    if (selected) {
        float pulse = 0.7f + 0.3f * std::sin(animTimer_ * 4.f);
        DrawText(">", cardX - 20, y + 15, 28, ColorAlpha(YELLOW, pulse));
    }

    // 槽位预览
    const char* slotDescs[] = {
        "P1(Red) AI(Red)  vs  P2(Blue) AI(Blue)",
        "P1(Atk) AI(Atk)  vs  P2(Def) AI(Def)",
        "P1  P2  AI  AI  (all independent)"
    };
    int slotW = MeasureText(slotDescs[index], 11);
    DrawText(slotDescs[index], cardX + (cardW - slotW) / 2, y + 100, 11,
             selected ? WHITE : GRAY);

    // AI 提示
    const char* aiNote = "AI types are always RANDOM";
    int aiW = MeasureText(aiNote, 11);
    DrawText(aiNote, cardX + (cardW - aiW) / 2, y + 118, 11,
             ColorAlpha(ORANGE, selected ? 1.f : 0.5f));
}

void ModeSelectScene::Render() {
    ClearBackground(BLACK);

    // 标题
    const char* title = "SELECT GAME MODE";
    int titleW = MeasureText(title, 36);
    DrawText(title, (GetScreenWidth() - titleW) / 2, 30, 36, GREEN);

    // 模式卡片
    int startY = 100;
    for (int i = 0; i < 3; i++) {
        RenderModeCard(i, startY + i * 160);
    }

    // 底部提示
    const char* hint = "UP/DOWN = Select  |  ENTER = Choose  |  ESC = Back";
    int hintW = MeasureText(hint, 16);
    float hintAlpha = 0.5f + 0.3f * std::sin(animTimer_ * 2.5f);
    DrawText(hint, (GetScreenWidth() - hintW) / 2, 750, 16, ColorAlpha(YELLOW, hintAlpha));
}
