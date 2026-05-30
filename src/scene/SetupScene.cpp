#include "SetupScene.h"
#include "SceneManager.h"
#include "MapSelectScene.h"
#include "game/tank/TankType.h"
#include "raylib.h"
#include <cstdio>

SetupScene::SetupScene(SceneManager* manager, GameMode mode)
    : manager_(manager), mode_(mode) {}

SetupScene::SetupScene(SceneManager* manager, GameSession session)
    : manager_(manager), mode_(session.GetGameMode()),
      session_(std::move(session)), hasSession_(true) {}

void SetupScene::Enter() {
    if (!hasSession_) {
        session_.SetGameMode(mode_);
    }
    currentSlot_ = 0;
}

void SetupScene::Update(float dt) {
    // 上下切换槽位
    if (IsKeyPressed(KEY_UP))   currentSlot_ = (currentSlot_ + 3) % 4;
    if (IsKeyPressed(KEY_DOWN)) currentSlot_ = (currentSlot_ + 1) % 4;

    // Tab 切换当前槽位的人/AI
    if (IsKeyPressed(KEY_TAB)) {
        session_.GetSlot(currentSlot_).isHuman = !session_.GetSlot(currentSlot_).isHuman;
        if (mode_ != GameMode::FREE_FOR_ALL) session_.BalanceAITeams();
    }

    auto& slot = session_.GetSlot(currentSlot_);

    if (mode_ == GameMode::FREE_FOR_ALL) {
        // FFA: 左右切换坦克类型
        if (slot.isHuman) {
            if (IsKeyPressed(KEY_LEFT)) {
                int t = (static_cast<int>(slot.tankType) + 3) % 4;
                slot.tankType = static_cast<TankType>(t);
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                int t = (static_cast<int>(slot.tankType) + 1) % 4;
                slot.tankType = static_cast<TankType>(t);
            }
        }
    } else {
        // 传统/攻防: 左右切换队伍 (仅人类可操作，可合作也可对抗)
        if (slot.isHuman && (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))) {
            slot.team = 1 - slot.team;
            session_.BalanceAITeams();
        }
    }

    // ENTER → 直接进地图选择 (队伍已在本界面配置)
    if (IsKeyPressed(KEY_ENTER)) {
        manager_->SetupNext(std::make_unique<MapSelectScene>(manager_, std::move(session_)));
    }

    // ESC → 返回模式选择
    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->PopScene();
    }
}

void SetupScene::Render() {
    ClearBackground(BLACK);
    int sw = GetScreenWidth();

    // 标题
    const char* title = "GAME SETUP";
    DrawText(title, (sw - MeasureText(title, 36)) / 2, 15, 36, GREEN);

    // 模式名 + 说明
    const char* modeNames[] = { "TRADITIONAL", "ATTACK / DEFEND", "FREE FOR ALL" };
    Color modeColors[] = { GREEN, RED, PURPLE };
    const char* modeDescs[] = {
        "2v2  |  2 Humans + 2 AI  |  Both teams have bases  |  No respawn",
        "2v2  |  2 Humans + 2 AI  |  Defenders respawn (3s)  |  1 base",
        "FFA  |  2 Humans + 2 AI  |  No bases  |  Last standing wins"
    };

    const char* mName = modeNames[static_cast<int>(mode_)];
    int mNameW = MeasureText(mName, 22);
    DrawText(mName, (sw - mNameW) / 2, 58, 22, modeColors[static_cast<int>(mode_)]);

    int descW = MeasureText(modeDescs[static_cast<int>(mode_)], 12);
    DrawText(modeDescs[static_cast<int>(mode_)],
             (sw - descW) / 2, 84, 12, GRAY);

    // 操作提示
    const char* ops;
    if (mode_ == GameMode::FREE_FOR_ALL) {
        ops = "UP/DOWN = Slot   TAB = Human/AI   LEFT/RIGHT = Tank Type";
    } else {
        ops = "UP/DOWN = Slot   TAB = Human/AI   LEFT/RIGHT = Team";
    }
    int opsW = MeasureText(ops, 13);
    DrawText(ops, (sw - opsW) / 2, 104, 13, LIGHTGRAY);

    // 槽位
    for (int i = 0; i < 4; i++) {
        RenderSlot(i);
    }

    // 下一步提示
    const char* next = "ENTER = Map Select   ESC = Back";
    int nextW = MeasureText(next, 16);
    DrawText(next, (sw - nextW) / 2, 760, 16, YELLOW);
}

void SetupScene::RenderSlot(int index) {
    const auto& slot = session_.GetSlot(index);
    int sw = GetScreenWidth();
    int boxW = 680;
    int boxX = (sw - boxW) / 2;
    int y = 130 + index * 150;

    bool selected = (index == currentSlot_);
    bool editable = slot.isHuman;

    // 卡片背景
    Color borderColor = selected ? YELLOW : DARKGRAY;
    if (selected) {
        DrawRectangle(boxX, y, boxW, 130, ColorAlpha(YELLOW, 0.06f));
    }
    DrawRectangleLines(boxX, y, boxW, 130, borderColor);

    // ---- 左半区: 编号 + 人/AI + 队伍 + 键位 ----
    int lx = boxX + 15;

    // 编号
    char buf[64];
    snprintf(buf, sizeof(buf), "P%d", index + 1);
    DrawText(buf, lx, y + 8, 28, borderColor);

    // 人/AI
    const char* typeStr = slot.isHuman ? "HUMAN" : "AI";
    Color typeColor = slot.isHuman ? GREEN : ORANGE;
    DrawText(typeStr, lx + 55, y + 12, 18, typeColor);

    // 队伍标签 (非FFA模式下人类可左右切换)
    const char* teamStr = nullptr;
    Color teamColor = WHITE;
    if (mode_ == GameMode::FREE_FOR_ALL) {
        static const char* ffaNames[] = { "RED", "BLUE", "GREEN", "PURPLE" };
        static Color ffaColors[] = { RED, BLUE, GREEN, PURPLE };
        teamStr = ffaNames[index];
        teamColor = ffaColors[index];
        DrawText(teamStr, lx, y + 38, 16, teamColor);
    } else {
        if (mode_ == GameMode::ATTACK_DEFEND) {
            teamStr = (slot.team == 0) ? "ATTACK" : "DEFEND";
            teamColor = (slot.team == 0) ? RED : BLUE;
        } else {
            teamStr = (slot.team == 0) ? "RED" : "BLUE";
            teamColor = (slot.team == 0) ? RED : BLUE;
        }
        if (slot.isHuman && selected) {
            DrawText("< ", lx, y + 38, 16, YELLOW);
            DrawText(teamStr, lx + 20, y + 38, 16, teamColor);
            DrawText(" >", lx + 20 + MeasureText(teamStr, 16) + 5, y + 38, 16, YELLOW);
        } else {
            DrawText(teamStr, lx, y + 38, 16, teamColor);
        }
    }

    // 命数 (攻防战)
    if (mode_ == GameMode::ATTACK_DEFEND) {
        const char* livesStr = (slot.team == 0) ? "3 lives" : "Infinite";
        Color livesColor = (slot.team == 0) ? YELLOW : GREEN;
        DrawText(livesStr, lx + 80, y + 40, 12, livesColor);
    }

    // 键位 (仅人类)
    if (slot.isHuman) {
        const char* keys = "";
        switch (index) {
            case 0: keys = "WASD+Space"; break;
            case 1: keys = "Arrows+Enter"; break;
            case 2: keys = "IJKL+U"; break;
            case 3: keys = "TFGH+R"; break;
        }
        DrawText(keys, lx, y + 60, 12, GRAY);
    }

    // ---- 右半区: 坦克类型 + 属性 ----
    int rx = boxX + 200;

    if (editable) {
        // 可编辑: 左右箭头选择
        const auto& stats = GetTankStats(slot.tankType);
        DrawText("< ", rx, y + 8, 24, selected ? YELLOW : DARKGRAY);
        DrawText(stats.name, rx + 25, y + 8, 24, WHITE);
        DrawText(" >", rx + 25 + MeasureText(stats.name, 24) + 8, y + 8, 24, selected ? YELLOW : DARKGRAY);

        // 属性数值
        char statBuf[128];
        snprintf(statBuf, sizeof(statBuf), "SPD:%.0f  HP:%d  ARM:%d  FIRE:%.1fs  BULLETS:%d",
                 stats.moveSpeed, stats.maxHealth, stats.armor, stats.fireCooldown, stats.maxBullets);
        DrawText(statBuf, rx, y + 42, 12, LIGHTGRAY);

        // 属性可视化条
        int barY = y + 60;
        int barW = 100;
        float alpha = selected ? 1.f : 0.5f;

        DrawRectangle(rx, barY, barW, 8, DARKGRAY);
        DrawRectangle(rx, barY, static_cast<int>(barW * stats.moveSpeed / 150.f), 8,
                      ColorAlpha(GREEN, alpha));
        DrawText("Speed", rx + barW + 5, barY - 2, 10, ColorAlpha(WHITE, alpha));

        int rx2 = rx + 170;
        DrawRectangle(rx2, barY, barW, 8, DARKGRAY);
        DrawRectangle(rx2, barY, static_cast<int>(barW * stats.maxHealth / 4.f), 8,
                      ColorAlpha(RED, alpha));
        DrawText("HP", rx2 + barW + 5, barY - 2, 10, ColorAlpha(WHITE, alpha));

        int rx3 = rx + 340;
        DrawRectangle(rx3, barY, barW, 8, DARKGRAY);
        DrawRectangle(rx3, barY, static_cast<int>(barW * stats.armor / 2.f), 8,
                      ColorAlpha(BLUE, alpha));
        DrawText("Armor", rx3 + barW + 5, barY - 2, 10, ColorAlpha(WHITE, alpha));

    } else {
        // AI: 显示 RANDOM
        DrawText("RANDOM", rx, y + 8, 24, ORANGE);
        DrawText("Type assigned at game start", rx, y + 40, 12, DARKGRAY);
        DrawText("LIGHT / MEDIUM / HEAVY / SPEED", rx, y + 58, 12, DARKGRAY);
    }
}
