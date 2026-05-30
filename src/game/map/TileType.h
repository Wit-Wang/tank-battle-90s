#pragma once

#include <cstdint>

/// 瓦片类型枚举
enum class TileType : uint8_t {
    EMPTY   = 0,  // 空地
    BRICK   = 1,  // 砖墙 (可破坏)
    STEEL   = 2,  // 钢墙 (不可破坏)
    WATER   = 3,  // 水面 (不可通行，子弹可穿过)
    GRASS   = 4,  // 草地 (可通行，遮挡视野)
    BASE    = 5,  // 基地 (老鹰)
};
