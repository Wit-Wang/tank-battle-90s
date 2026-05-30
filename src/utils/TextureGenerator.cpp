#include "TextureGenerator.h"
#include "core/ResourceManager.h"
#include "core/Types.h"
#include <cstdlib>
#include <cmath>

// ---- 颜色调色板 ----

static constexpr Color RED_DARK    = {160, 40, 40, 255};
static constexpr Color RED_MID     = {200, 60, 60, 255};
static constexpr Color RED_LIGHT   = {230, 100, 100, 255};
static constexpr Color RED_HIGHLIGHT = {255, 150, 150, 255};

static constexpr Color BLUE_DARK   = {30, 50, 150, 255};
static constexpr Color BLUE_MID    = {50, 80, 200, 255};
static constexpr Color BLUE_LIGHT  = {80, 120, 230, 255};
static constexpr Color BLUE_HIGHLIGHT = {140, 170, 255, 255};

static constexpr Color METAL_DARK  = {80, 80, 90, 255};
static constexpr Color METAL_MID   = {120, 120, 135, 255};
static constexpr Color METAL_LIGHT = {160, 160, 175, 255};
static constexpr Color TREAD_DARK  = {50, 45, 40, 255};
static constexpr Color TREAD_MID   = {90, 80, 70, 255};
static constexpr Color YELLOW_BUL  = {255, 230, 60, 255};

static Color TeamDark(int t)  { return t == 0 ? RED_DARK  : BLUE_DARK; }
static Color TeamMid(int t)   { return t == 0 ? RED_MID   : BLUE_MID; }
static Color TeamLight(int t) { return t == 0 ? RED_LIGHT  : BLUE_LIGHT; }

// ---- 绘制辅助 ----

static void DrawRect(Image& img, int x, int y, int w, int h, Color c) {
    ImageDrawRectangle(&img, x, y, w, h, c);
}

static void DrawPx(Image& img, int x, int y, Color c) {
    ImageDrawPixel(&img, x, y, c);
}

static void FillCircle(Image& img, int cx, int cy, int r, Color c) {
    ImageDrawCircle(&img, cx, cy, r, c);
}

static void DrawRectBordered(Image& img, int x, int y, int w, int h, Color fill, Color border) {
    DrawRect(img, x, y, w, h, fill);
    DrawRect(img, x, y, w, 1, border);
    DrawRect(img, x, y + h - 1, w, 1, border);
    DrawRect(img, x, y, 1, h, border);
    DrawRect(img, x + w - 1, y, 1, h, border);
}

static void AddHighlight(Image& img, int x, int y, int w, int h, Color hl) {
    DrawRect(img, x + 1, y + 1, w - 2, 1, hl);
    DrawRect(img, x + 1, y + 1, 1, h - 2, hl);
}

static void AddShadow(Image& img, int x, int y, int w, int h, Color sh) {
    DrawRect(img, x + 1, y + h - 2, w - 2, 1, sh);
    DrawRect(img, x + w - 2, y + 1, 1, h - 2, sh);
}

// ---- 坦克部件 ----

static void DrawTreads(Image& img, int bodyY, int team) {
    int cx = 32;
    // 左履带
    int lx = cx - 16;
    DrawRect(img, lx, bodyY, 8, 36, TREAD_DARK);
    for (int i = 0; i < 36; i += 4)
        DrawRect(img, lx, bodyY + i, 8, 2, TREAD_MID);
    DrawRect(img, lx, bodyY, 1, 36, {40, 35, 30, 255});
    DrawRect(img, lx + 7, bodyY, 1, 36, {40, 35, 30, 255});

    // 右履带
    int rx = cx + 8;
    DrawRect(img, rx, bodyY, 8, 36, TREAD_DARK);
    for (int i = 0; i < 36; i += 4)
        DrawRect(img, rx, bodyY + i, 8, 2, TREAD_MID);
    DrawRect(img, rx, bodyY, 1, 36, {40, 35, 30, 255});
    DrawRect(img, rx + 7, bodyY, 1, 36, {40, 35, 30, 255});
}

static void DrawTurret(Image& img, int cx, int cy, int r) {
    FillCircle(img, cx, cy, r + 1, METAL_DARK);
    FillCircle(img, cx, cy, r, METAL_MID);
    FillCircle(img, cx - 1, cy - 1, r - 2, METAL_LIGHT);
    DrawPx(img, cx - 1, cy - 1, {200, 200, 210, 255});
}

static void DrawBarrel(Image& img, int cx, int cy, int length, Color color) {
    int bw = 4;
    DrawRect(img, cx - bw / 2, cy - length, bw, length, color);
    DrawRect(img, cx - bw / 2, cy - length, 1, length, {180, 180, 190, 255});
    DrawRect(img, cx - bw / 2 - 1, cy - length, bw + 2, 3, METAL_DARK);
}

// ======================
//   坦克纹理
// ======================

Texture2D TextureGenerator::GenerateTank(TankType type, int team) {
    Image img = GenImageColor(64, 64, BLANK);

    int cx = 32, cy = 32;

    int bodyW, bodyH, barrelLen, turretR;
    switch (type) {
        case TankType::LIGHT:  bodyW = 28; bodyH = 32; barrelLen = 20; turretR = 7; break;
        case TankType::MEDIUM: bodyW = 32; bodyH = 36; barrelLen = 22; turretR = 8; break;
        case TankType::HEAVY:  bodyW = 38; bodyH = 40; barrelLen = 18; turretR = 10; break;
        case TankType::SPEED:  bodyW = 24; bodyH = 30; barrelLen = 24; turretR = 6; break;
        default:               bodyW = 32; bodyH = 36; barrelLen = 22; turretR = 8; break;
    }

    int bodyX = cx - bodyW / 2;
    int bodyY = cy - bodyH / 2 + 2;

    DrawTreads(img, bodyY, team);
    DrawRectBordered(img, bodyX + 4, bodyY, bodyW - 8, bodyH, TeamMid(team), TeamDark(team));
    AddHighlight(img, bodyX + 4, bodyY, bodyW - 8, bodyH, TeamLight(team));
    AddShadow(img, bodyX + 4, bodyY, bodyW - 8, bodyH, TeamDark(team));
    DrawRect(img, bodyX + 6, bodyY + bodyH / 2, bodyW - 12, 2, TeamDark(team));
    DrawTurret(img, cx, cy + 2, turretR);
    DrawBarrel(img, cx, cy + 2, barrelLen, METAL_MID);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// ======================
//   子弹纹理
// ======================

Texture2D TextureGenerator::GenerateBullet(int team) {
    Image img = GenImageColor(8, 8, BLANK);

    FillCircle(img, 4, 4, 3, YELLOW_BUL);
    FillCircle(img, 4, 4, 2, {255, 255, 200, 255});
    DrawPx(img, 3, 3, WHITE);
    DrawPx(img, 4, 3, WHITE);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// ======================
//   地砖纹理
// ======================

Texture2D TextureGenerator::GenerateTile(TileType type) {
    Image img = GenImageColor(64, 64, BLANK);

    switch (type) {
        case TileType::BRICK: {
            Color brick = {180, 90, 50, 255};
            Color mortar = {140, 130, 110, 255};
            Color brickDark = {140, 65, 35, 255};
            Color brickLight = {210, 120, 75, 255};

            DrawRect(img, 0, 0, 64, 64, brick);
            for (int y = 0; y < 64; y += 8)
                DrawRect(img, 0, y, 64, 1, mortar);
            for (int y = 0; y < 64; y += 8) {
                int offset = ((y / 8) % 2 == 0) ? 0 : 16;
                for (int x = offset; x < 64; x += 32)
                    DrawRect(img, x, y, 1, 8, mortar);
            }
            for (int row = 0; row < 8; row++) {
                int offset = (row % 2 == 0) ? 0 : 16;
                for (int col = 0; col < 3; col++) {
                    int bx = offset + col * 32 + 1;
                    int by = row * 8 + 1;
                    if (bx < 63 && by < 63) {
                        DrawRect(img, bx, by, 14, 1, brickLight);
                        DrawRect(img, bx, by, 1, 6, brickLight);
                        DrawRect(img, bx + 14, by, 1, 6, brickDark);
                        DrawRect(img, bx, by + 5, 15, 1, brickDark);
                    }
                }
            }
            break;
        }
        case TileType::STEEL: {
            Color steelBase = {170, 175, 185, 255};
            Color steelLight = {210, 215, 225, 255};
            Color steelDark = {110, 115, 125, 255};
            Color rivet = {90, 95, 105, 255};

            DrawRect(img, 0, 0, 64, 64, steelBase);
            DrawRect(img, 0, 31, 64, 2, steelDark);
            DrawRect(img, 31, 0, 2, 64, steelDark);
            DrawRect(img, 1, 1, 29, 1, steelLight);
            DrawRect(img, 1, 1, 1, 29, steelLight);
            DrawRect(img, 34, 34, 29, 1, steelLight);
            DrawRect(img, 34, 34, 1, 29, steelLight);
            DrawRect(img, 1, 29, 30, 1, steelDark);
            DrawRect(img, 29, 1, 1, 30, steelDark);
            DrawRect(img, 34, 62, 29, 1, steelDark);
            DrawRect(img, 62, 34, 1, 29, steelDark);

            int rivets[][2] = {{6,6},{26,6},{6,26},{26,26},{37,37},{57,37},{37,57},{57,57}};
            for (auto& r : rivets) {
                DrawRect(img, r[0], r[1], 3, 3, rivet);
                DrawPx(img, r[0], r[1], steelLight);
            }
            break;
        }
        case TileType::WATER: {
            Color waterBase = {30, 80, 160, 255};
            Color waterLight = {60, 120, 200, 255};
            Color waterDark = {20, 50, 120, 255};
            Color waterHL = {100, 160, 230, 255};

            DrawRect(img, 0, 0, 64, 64, waterBase);
            for (int y = 0; y < 64; y += 16) {
                int offset = ((y / 16) % 2 == 0) ? 0 : 20;
                for (int x = offset; x < 64; x += 40) {
                    for (int i = 0; i < 20 && x + i < 64; i++) {
                        int waveY = y + (i < 10 ? i : 20 - i);
                        if (waveY < 64) {
                            DrawPx(img, x + i, waveY, waterLight);
                            if (waveY + 1 < 64)
                                DrawPx(img, x + i, waveY + 1, waterHL);
                        }
                    }
                }
            }
            for (int y = 8; y < 64; y += 16)
                DrawRect(img, 0, y, 64, 2, waterDark);
            break;
        }
        case TileType::GRASS: {
            Color grassBase = {40, 120, 40, 255};
            Color grassLight = {60, 160, 55, 255};
            Color grassDark = {25, 90, 25, 255};
            Color grassHL = {80, 180, 75, 255};

            DrawRect(img, 0, 0, 64, 64, grassBase);
            for (int i = 0; i < 80; i++) {
                int px = (i * 37 + 13) % 62 + 1;
                int py = (i * 53 + 7) % 62 + 1;
                Color c = (i % 3 == 0) ? grassLight : ((i % 3 == 1) ? grassDark : grassHL);
                DrawPx(img, px, py, c);
                if (px + 1 < 64) DrawPx(img, px + 1, py, c);
                if (py + 1 < 64) DrawPx(img, px, py + 1, c);
            }
            for (int i = 0; i < 12; i++) {
                int cx = (i * 19 + 5) % 56 + 4;
                int cy = (i * 31 + 11) % 56 + 4;
                DrawRect(img, cx, cy, 3, 2, grassLight);
                DrawPx(img, cx + 1, cy - 1, grassHL);
            }
            break;
        }
        case TileType::BASE: {
            DrawRect(img, 0, 0, 64, 64, {60, 50, 30, 255});
            DrawRect(img, 2, 2, 60, 60, {80, 70, 40, 255});
            DrawRect(img, 0, 0, 64, 3, GOLD);
            DrawRect(img, 0, 61, 64, 3, GOLD);
            DrawRect(img, 0, 0, 3, 64, GOLD);
            DrawRect(img, 61, 0, 3, 64, GOLD);
            break;
        }
        case TileType::EMPTY:
        default:
            DrawRect(img, 0, 0, 64, 64, {30, 30, 35, 255});
            break;
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// ======================
//   基地纹理
// ======================

Texture2D TextureGenerator::GenerateBase(int team, bool destroyed) {
    Image img = GenImageColor(64, 64, BLANK);

    if (destroyed) {
        DrawRect(img, 0, 0, 64, 64, {50, 45, 40, 255});
        for (int i = 0; i < 20; i++) {
            int px = (i * 17 + 3) % 58 + 3;
            int py = (i * 29 + 7) % 58 + 3;
            DrawRect(img, px, py, (i % 3) + 3, (i % 2) + 2, {70, 60, 50, 255});
        }
        for (int i = 0; i < 15; i++) {
            int px = (i * 23 + 11) % 60 + 2;
            int py = (i * 41 + 13) % 60 + 2;
            DrawPx(img, px, py, {40, 35, 30, 255});
        }
    } else {
        Color baseColor = team == 0 ? RED_MID : BLUE_MID;
        Color baseDark = team == 0 ? RED_DARK : BLUE_DARK;
        Color baseLight = team == 0 ? RED_LIGHT : BLUE_LIGHT;

        DrawRect(img, 0, 0, 64, 64, {60, 55, 45, 255});
        DrawRectBordered(img, 3, 3, 58, 58, {75, 70, 55, 255}, {50, 45, 35, 255});

        int cx = 32, cy = 32;
        for (int r = 0; r < 16; r++) {
            for (int dy = -r; dy <= r; dy++) {
                int maxX = r - std::abs(dy);
                for (int dx = -maxX; dx <= maxX; dx++) {
                    int px = cx + dx;
                    int py = cy + dy;
                    if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                        Color c = (std::abs(dx) + std::abs(dy) < r * 0.6) ? baseLight : baseColor;
                        DrawPx(img, px, py, c);
                    }
                }
            }
        }
        for (int i = -16; i <= 16; i++) {
            int ax = std::abs(i);
            DrawPx(img, cx + i, cy - (16 - ax), baseDark);
            DrawPx(img, cx + i, cy + (16 - ax), baseDark);
            DrawPx(img, cx - (16 - ax), cy + i, baseDark);
            DrawPx(img, cx + (16 - ax), cy + i, baseDark);
        }

        DrawRect(img, 6, 6, 4, 4, baseColor);
        DrawRect(img, 54, 6, 4, 4, baseColor);
        DrawRect(img, 6, 54, 4, 4, baseColor);
        DrawRect(img, 54, 54, 4, 4, baseColor);
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// ======================
//   批量生成并注册
// ======================

void TextureGenerator::GenerateAll() {
    auto& rm = ResourceManager::Instance();

    const TankType types[] = { TankType::LIGHT, TankType::MEDIUM, TankType::HEAVY, TankType::SPEED };
    const char* typeNames[] = { "light", "medium", "heavy", "speed" };
    for (int t = 0; t < 4; t++) {
        for (int team = 0; team < 2; team++) {
            std::string name = std::string("tank_") + typeNames[t] + "_team" + std::to_string(team);
            rm.RegisterTexture(name, GenerateTank(types[t], team));
        }
    }

    for (int team = 0; team < 2; team++) {
        std::string name = std::string("bullet_team") + std::to_string(team);
        rm.RegisterTexture(name, GenerateBullet(team));
    }

    const TileType tiles[] = { TileType::EMPTY, TileType::BRICK, TileType::STEEL, TileType::WATER, TileType::GRASS, TileType::BASE };
    const char* tileNames[] = { "tile_empty", "tile_brick", "tile_steel", "tile_water", "tile_grass", "tile_base" };
    for (int i = 0; i < 6; i++)
        rm.RegisterTexture(tileNames[i], GenerateTile(tiles[i]));

    for (int team = 0; team < 2; team++) {
        rm.RegisterTexture(std::string("base_team") + std::to_string(team), GenerateBase(team, false));
        rm.RegisterTexture(std::string("base_team") + std::to_string(team) + "_destroyed", GenerateBase(team, true));
    }
}
