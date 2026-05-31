#include "HUD.h"
#include "game/session/GameSession.h"
#include "game/tank/Tank.h"
#include "core/Types.h"
#include "raylib.h"
#include <cstdio>

HUD::HUD(const GameSession& session)
    : session_(session), mode_(session.GetGameMode()) {}

void HUD::Render() {
    switch (mode_) {
        case GameMode::TRADITIONAL:   RenderTraditional();   break;
        case GameMode::ATTACK_DEFEND: RenderAttackDefend();  break;
        case GameMode::FREE_FOR_ALL:  RenderFreeForAll();    break;
    }
}

void HUD::RenderTraditional() {
    int panelW = 180;
    int panelH = 160;
    int panelX = (GetScreenWidth() - panelW) / 2;
    int panelY = 8;

    DrawRectangle(panelX, panelY, panelW, panelH, ColorAlpha(BLACK, 0.7f));
    DrawRectangleLines(panelX, panelY, panelW, panelH, DARKGRAY);

    int textX = panelX + 10;
    int y = panelY + 8;
    int lineH = 20;

    int teamAlive[2] = {0, 0};
    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        const auto& slot = session_.GetSlot(i);
        if (slot.tank && !slot.tank->IsDead()) {
            teamAlive[slot.team]++;
        }
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "RED: %d", teamAlive[0]);
    DrawText(buf, textX, y, 16, RED);
    y += lineH;

    snprintf(buf, sizeof(buf), "BLUE: %d", teamAlive[1]);
    DrawText(buf, textX, y, 16, BLUE);
    y += lineH + 5;

    DrawLine(panelX + 5, y, panelX + panelW - 5, y, DARKGRAY);
    y += 5;

    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        const auto& slot = session_.GetSlot(i);
        Color color = slot.GetTeamColor();
        const char* pType = slot.isHuman ? "P" : "AI";

        const char* status = slot.tank ? (slot.tank->IsDead() ? "DEAD" : "OK") : "---";
        snprintf(buf, sizeof(buf), "%s%d %s", pType, i + 1, status);
        DrawText(buf, textX, y, 14, color);
        y += lineH;
    }
}

void HUD::RenderAttackDefend() {
    int panelW = 220;
    int panelH = 200;
    int panelX = (GetScreenWidth() - panelW) / 2;
    int panelY = 8;

    DrawRectangle(panelX, panelY, panelW, panelH, ColorAlpha(BLACK, 0.7f));
    DrawRectangleLines(panelX, panelY, panelW, panelH, DARKGRAY);

    int textX = panelX + 10;
    int y = panelY + 8;
    int lineH = 20;

    char buf[64];

    // 攻方状态 (共享命)
    int atkLives = session_.GetSharedLives(0);
    snprintf(buf, sizeof(buf), "ATTACK (Lives: %d):", atkLives);
    DrawText(buf, textX, y, 14, RED);
    y += lineH;
    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        const auto& slot = session_.GetSlot(i);
        if (slot.team != 0) continue;

        const char* pType = slot.isHuman ? "P" : "AI";
        const char* status;
        if (slot.tank && !slot.tank->IsDead()) {
            status = "ALIVE";
        } else if (slot.isRespawning) {
            snprintf(buf, sizeof(buf), "RESPAWN %.1fs", slot.respawnTimer);
            status = buf;
        } else {
            status = "OUT";
        }
        snprintf(buf, sizeof(buf), "  %s%d: %s", pType, i + 1, status);
        DrawText(buf, textX, y, 12, RED);
        y += 16;
    }

    y += 4;
    DrawLine(panelX + 5, y, panelX + panelW - 5, y, DARKGRAY);
    y += 4;

    // 守方状态 (无限命)
    DrawText("DEFEND (Lives: INF):", textX, y, 14, BLUE);
    y += lineH;
    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        const auto& slot = session_.GetSlot(i);
        if (slot.team != 1) continue;

        const char* pType = slot.isHuman ? "P" : "AI";
        const char* status;
        if (slot.tank && !slot.tank->IsDead()) {
            status = "ALIVE";
        } else if (slot.isRespawning) {
            snprintf(buf, sizeof(buf), "RESPAWN %.1fs", slot.respawnTimer);
            status = buf;
        } else {
            status = "DEAD";
        }
        snprintf(buf, sizeof(buf), "  %s%d: %s", pType, i + 1, status);
        DrawText(buf, textX, y, 12, BLUE);
        y += 16;
    }
}

void HUD::RenderFreeForAll() {
    int panelW = 180;
    int panelH = 140;
    int panelX = (GetScreenWidth() - panelW) / 2;
    int panelY = 8;

    DrawRectangle(panelX, panelY, panelW, panelH, ColorAlpha(BLACK, 0.7f));
    DrawRectangleLines(panelX, panelY, panelW, panelH, DARKGRAY);

    int textX = panelX + 10;
    int y = panelY + 8;
    int lineH = 20;

    static Color ffaColors[] = { RED, BLUE, GREEN, PURPLE };
    static const char* ffaNames[] = { "RED", "BLUE", "GREEN", "PURPLE" };

    DrawText("FREE FOR ALL", textX, y, 14, PURPLE);
    y += lineH + 4;

    int aliveCount = 0;
    char buf[64];
    for (int i = 0; i < GameSession::SLOT_COUNT; i++) {
        const auto& slot = session_.GetSlot(i);
        const char* pType = slot.isHuman ? "P" : "AI";
        const char* status = (slot.tank && !slot.tank->IsDead()) ? "OK" : "DEAD";

        if (slot.tank && !slot.tank->IsDead()) aliveCount++;

        snprintf(buf, sizeof(buf), "%s%d [%s] %s", pType, i + 1, ffaNames[i], status);
        DrawText(buf, textX, y, 13, ffaColors[i]);
        y += 18;
    }

    // 剩余存活
    snprintf(buf, sizeof(buf), "Alive: %d", aliveCount);
    DrawText(buf, textX, y + 4, 14, YELLOW);
}
