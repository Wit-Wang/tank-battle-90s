// winsock2.h 必须在 windows.h (raylib) 之前包含
#include "net/NetworkManager.h"
#include "net/NetController.h"

#include "GameNetworkHost.h"
#include "game/tank/Tank.h"
#include "game/map/GameMap.h"
#include "game/entities/Bullet.h"
#include "ecs/EntityManager.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/HealthComponent.h"
#include "core/Types.h"

void GameNetworkHost::Init(GameMap& map) {
    if (!net_) return;

    net_->BindUDP(NET_DEFAULT_PORT);

    // 保存地图快照 (用于变化检测)
    prevMapTiles_.resize(MAP_ROWS, std::vector<TileType>(MAP_COLS));
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            prevMapTiles_[r][c] = map.GetTile(c, r);
        }
    }

    broadcastTimer_ = 0.f;
    stateSeq_ = 0;
}

void GameNetworkHost::BroadcastState(const std::vector<std::unique_ptr<Tank>>& tanks,
                                      EntityManager& em) {
    if (!net_ || net_->ClientCount() == 0) return;

    NetMessage state(NetMessageType::GameState);
    state.WritePayload(stateSeq_++);

    // 4 个玩家状态
    for (int i = 0; i < 4; i++) {
        NetPlayerState ps{};
        if (i < static_cast<int>(tanks.size()) && tanks[i] && tanks[i]->GetEntity()) {
            auto* t = tanks[i]->GetEntity()->GetComponent<TransformComponent>();
            auto* h = tanks[i]->GetEntity()->GetComponent<HealthComponent>();
            if (t) {
                ps.x = t->GetPosition().x;
                ps.y = t->GetPosition().y;
                ps.rotation = t->GetRotation();
            }
            if (h) {
                ps.hp = static_cast<uint8_t>(h->GetHp());
                ps.maxHp = static_cast<uint8_t>(h->GetMaxHp());
            }
            ps.alive = tanks[i]->IsDead() ? 0 : 1;
            ps.invincible = tanks[i]->IsInvincible() ? 1 : 0;
        }
        state.WritePayload(ps);
    }

    // 子弹列表
    std::vector<NetBulletState> bulletStates;
    em.ForEach([&](Entity* e) {
        auto* bullet = dynamic_cast<Bullet*>(e);
        if (bullet && bullet->IsActive()) {
            NetBulletState bs{};
            bs.bulletId = static_cast<uint8_t>(bullet->GetId() & 0xFF);
            auto* t = bullet->GetComponent<TransformComponent>();
            if (t) {
                bs.x = t->GetPosition().x;
                bs.y = t->GetPosition().y;
            }
            bs.ownerTeam = static_cast<uint8_t>(bullet->GetOwnerTeam());
            bs.active = 1;
            bulletStates.push_back(bs);
        }
    });

    state.WritePayload(static_cast<uint8_t>(bulletStates.size()));
    for (auto& bs : bulletStates) {
        state.WritePayload(bs);
    }

    net_->BroadcastTCP(state);
}

void GameNetworkHost::CheckMapChanges(GameMap& map) {
    if (prevMapTiles_.empty() || !net_) return;

    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            TileType current = map.GetTile(c, r);
            if (current != prevMapTiles_[r][c]) {
                NetMessage wallMsg(NetMessageType::WallChanged);
                NetWallChangedPayload wp;
                wp.col = static_cast<uint8_t>(c);
                wp.row = static_cast<uint8_t>(r);
                wp.newType = static_cast<uint8_t>(current);
                wallMsg.WritePayload(wp);
                net_->BroadcastTCP(wallMsg);

                prevMapTiles_[r][c] = current;
            }
        }
    }
}

void GameNetworkHost::BroadcastGameOver(int winningTeam) {
    if (!net_) return;
    NetMessage overMsg(NetMessageType::GameOver);
    overMsg.WritePayload(static_cast<uint8_t>(winningTeam));
    net_->BroadcastTCP(overMsg);
}
