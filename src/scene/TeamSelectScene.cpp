#include "TeamSelectScene.h"
#include "SceneManager.h"
#include "SetupScene.h"
#include "MapSelectScene.h"
#include "raylib.h"

TeamSelectScene::TeamSelectScene(SceneManager* manager, GameSession session)
    : manager_(manager), session_(std::move(session)),
      mode_(session_.GetGameMode()) {}

void TeamSelectScene::Enter() {
    currentSlot_ = 0;
}

void TeamSelectScene::Update(float dt) {
    if (IsKeyPressed(KEY_UP))   currentSlot_ = (currentSlot_ + 3) % 4;
    if (IsKeyPressed(KEY_DOWN)) currentSlot_ = (currentSlot_ + 1) % 4;

    // 左右切换队伍 (传统对战模式)
    if (mode_ == GameMode::TRADITIONAL) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
            auto& slot = session_.GetSlot(currentSlot_);
            slot.team = 1 - slot.team;  // 0 ↔ 1
        }
    }
    // 攻防战: 也可以切换 (让用户选择当攻方还是守方)
    // 但要确保 1攻 + 3守 的配置
    if (mode_ == GameMode::ATTACK_DEFEND) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
            auto& slot = session_.GetSlot(currentSlot_);
            int oldTeam = slot.team;
            slot.team = 1 - slot.team;

            // 检查: 攻方(team0)必须恰好 1 人
            int atkCount = 0;
            for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
                if (session_.GetSlot(i).team == 0) atkCount++;
            }
            // 如果攻方人数不是1，回退
            if (atkCount != 1) {
                slot.team = oldTeam;
            }
        }
    }

    if (IsKeyPressed(KEY_ENTER)) {
        manager_->PostSetupNext(std::make_unique<MapSelectScene>(manager_, std::move(session_)));
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        manager_->PostSetupNext(std::make_unique<SetupScene>(manager_, mode_));
    }
}

void TeamSelectScene::Render() {
    ClearBackground(BLACK);

    DrawText("SELECT TEAM", (GetScreenWidth() - MeasureText("SELECT TEAM", 36)) / 2, 30, 36, GREEN);

    if (mode_ == GameMode::TRADITIONAL) {
        DrawText("UP/DOWN = Slot  |  LEFT/RIGHT = Switch team",
                 (GetScreenWidth() - MeasureText("UP/DOWN = Slot  |  LEFT/RIGHT = Switch team", 14)) / 2,
                 80, 14, GRAY);
    } else if (mode_ == GameMode::ATTACK_DEFEND) {
        DrawText("UP/DOWN = Slot  |  LEFT/RIGHT = Switch (1 ATK + 3 DEF required)",
                 60, 80, 14, GRAY);
    }

    // 队伍统计
    int team0Count = 0, team1Count = 0;
    for (int i = 0; i < 4; i++) {
        if (session_.GetSlot(i).team == 0) team0Count++;
        else team1Count++;
    }

    char buf[64];
    if (mode_ == GameMode::ATTACK_DEFEND) {
        snprintf(buf, sizeof(buf), "ATTACK: %d", team0Count);
        DrawText(buf, 200, 110, 22, RED);
        snprintf(buf, sizeof(buf), "DEFEND: %d", team1Count);
        DrawText(buf, 500, 110, 22, BLUE);
        // 攻防战命提示
        DrawText("Attackers: 3 lives  |  Defenders: Infinite (3s respawn)", 140, 138, 13, YELLOW);
    } else {
        snprintf(buf, sizeof(buf), "RED: %d", team0Count);
        DrawText(buf, 200, 110, 22, RED);
        snprintf(buf, sizeof(buf), "BLUE: %d", team1Count);
        DrawText(buf, 500, 110, 22, BLUE);
    }

    for (int i = 0; i < 4; i++) {
        const auto& slot = session_.GetSlot(i);
        int y = 160 + i * 120;

        Color highlight = (i == currentSlot_) ? YELLOW : WHITE;
        DrawRectangleLines(80, y, 650, 90, highlight);

        // 槽位
        snprintf(buf, sizeof(buf), "P%d", i + 1);
        DrawText(buf, 100, y + 10, 28, highlight);

        // 人/AI
        const char* typeStr = slot.isHuman ? "HUMAN" : "AI";
        DrawText(typeStr, 160, y + 15, 18, slot.isHuman ? GREEN : ORANGE);

        // 坦克类型
        if (slot.isHuman) {
            const auto& stats = GetTankStats(slot.tankType);
            DrawText(stats.name, 260, y + 15, 20, WHITE);
        } else {
            DrawText("RANDOM", 260, y + 15, 20, ORANGE);
        }

        // 队伍选择
        const char* teamStr;
        Color teamColor = (slot.team == 0) ? RED : BLUE;
        if (mode_ == GameMode::ATTACK_DEFEND) {
            teamStr = (slot.team == 0) ? "ATTACK" : "DEFEND";
        } else {
            teamStr = (slot.team == 0) ? "RED" : "BLUE";
        }

        int teamW = MeasureText(teamStr, 28);
        DrawText("< ", 450, y + 10, 28, YELLOW);
        DrawText(teamStr, 480, y + 10, 28, teamColor);
        DrawText(" >", 480 + teamW + 10, y + 10, 28, YELLOW);

        // 模式特定提示
        if (mode_ == GameMode::ATTACK_DEFEND) {
            if (slot.team == 0) {
                DrawText("Destroy enemy base  |  3 lives", 260, y + 50, 13, RED);
            } else {
                DrawText("Defend your base  |  Infinite respawn", 260, y + 50, 13, BLUE);
            }
        } else {
            if (slot.team == 0) {
                DrawText("TARGET: Destroy enemy base", 260, y + 50, 14, RED);
            } else {
                DrawText("TARGET: Defend your base", 260, y + 50, 14, BLUE);
            }
        }
    }

    DrawText("ENTER = Next (Map Select)  |  ESC = Back", 170, 750, 18, YELLOW);
}
