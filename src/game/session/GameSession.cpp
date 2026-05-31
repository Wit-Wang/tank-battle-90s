#include "GameSession.h"
#include "game/map/MapManager.h"
#include "utils/Random.h"

GameSession::GameSession() {
    SetGameMode(GameMode::TRADITIONAL);
}

void GameSession::SetGameMode(GameMode mode) {
    mode_ = mode;

    // 重置所有槽位
    for (auto& s : slots_) {
        s = PlayerSlot{};
    }

    switch (mode) {
        case GameMode::TRADITIONAL:
            // 传统对战: 2真人+2AI, 默认同队合作 (可左右切换队伍)
            slots_[0] = { true,  TankType::MEDIUM, 0, KEY_W, KEY_S, KEY_A, KEY_D, KEY_SPACE };
            slots_[1] = { true,  TankType::MEDIUM, 0, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER };
            slots_[2] = { false, TankType::MEDIUM, 1 };
            slots_[3] = { false, TankType::MEDIUM, 1 };
            break;

        case GameMode::ATTACK_DEFEND:
            // 攻防战: 2真人+2AI, 默认同队合作 (可左右切换队伍)
            slots_[0] = { true,  TankType::MEDIUM, 0, KEY_W, KEY_S, KEY_A, KEY_D, KEY_SPACE };
            slots_[1] = { true,  TankType::MEDIUM, 0, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER };
            slots_[2] = { false, TankType::MEDIUM, 1 };
            slots_[3] = { false, TankType::MEDIUM, 1 };
            break;

        case GameMode::FREE_FOR_ALL:
            // 各自为战: 4人各自独立, 每人一个队伍
            slots_[0] = { true,  TankType::MEDIUM, 0, KEY_W, KEY_S, KEY_A, KEY_D, KEY_SPACE };
            slots_[1] = { true,  TankType::MEDIUM, 1, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER };
            slots_[2] = { false, TankType::MEDIUM, 2 };
            slots_[3] = { false, TankType::MEDIUM, 3 };
            break;
    }

    // 设置命 (攻防战使用共享命池)
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (mode == GameMode::FREE_FOR_ALL) {
            slots_[i].lives = 1;  // 一次命
        } else {
            slots_[i].lives = DEFAULT_LIVES;
        }
    }
    if (mode == GameMode::ATTACK_DEFEND) {
        sharedLives_[0] = AD_ATTACKER_LIVES;  // 攻方共享命 (有限)
        sharedLives_[1] = -1;                   // 守方无限命
    } else {
        sharedLives_[0] = 0;
        sharedLives_[1] = 0;
    }
}

void GameSession::BalanceAITeams() {
    // Count humans per team
    int count[2] = {0, 0};
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (slots_[i].isHuman) count[slots_[i].team]++;
    }
    // Assign AI slots to the team that needs more members (target: 2 per team)
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (!slots_[i].isHuman) {
            slots_[i].team = (count[0] <= count[1]) ? 0 : 1;
            count[slots_[i].team]++;
        }
    }
}

void GameSession::RandomizeAITypes() {
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (!slots_[i].isHuman) {
            slots_[i].tankType = static_cast<TankType>(Random::Int(0, 3));
        }
    }
}

void GameSession::ApplyMapSelection(const MapManager& mm) {
    mapPath_ = mm.GetCurrentMapInfo().filePath;
}

PlayerSlot& GameSession::GetSlot(int index) {
    return slots_[index];
}

const PlayerSlot& GameSession::GetSlot(int index) const {
    return slots_[index];
}
