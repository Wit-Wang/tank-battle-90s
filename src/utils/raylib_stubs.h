#pragma once

// Minimal raylib stubs for headless server builds (no X11/OpenGL required).
// Provides types, constants, and function stubs that the shared game logic needs.

#ifndef TANKGAME_SERVER
#error "raylib_stubs.h should only be included in server builds (TANKGAME_SERVER defined)"
#endif

#include <cmath>
#include <cstring>

// ---- Types ----

struct Vector2 { float x, y; };
struct Rectangle { float x, y, width, height; };
struct Color { unsigned char r, g, b, a; };
struct Texture2D { int id, width, height, mipmaps, format; };
struct Sound { unsigned int frameCount; };

// ---- Constants ----

static constexpr float DEG2RAD = 0.017453292519943295769236907684886f;

// Colors
static const Color WHITE      = { 255, 255, 255, 255 };
static const Color BLACK      = {   0,   0,   0, 255 };
static const Color BROWN      = { 160, 120,  40, 255 };
static const Color DARKBROWN  = { 100,  70,  20, 255 };
static const Color LIGHTGRAY  = { 200, 200, 200, 255 };
static const Color DARKGRAY   = {  80,  80,  80, 255 };
static const Color DARKGREEN  = {   0, 100,   0, 255 };
static const Color BLUE       = {   0, 121, 241, 255 };
static const Color GREEN      = {   0, 228,  48, 255 };
static const Color YELLOW     = { 253, 249,   0, 255 };
static const Color RED        = { 230,  41,  55, 255 };
static const Color ORANGE     = { 255, 161,   0, 255 };
static const Color GRAY       = { 130, 130, 130, 255 };
static const Color GOLD       = { 255, 203,   0, 255 };
static const Color SKYBLUE    = { 102, 191, 255, 255 };
static const Color PURPLE     = { 200, 122, 255, 255 };

// Key codes
static const int KEY_W     = 87;
static const int KEY_A     = 65;
static const int KEY_S     = 83;
static const int KEY_D     = 68;
static const int KEY_SPACE = 32;
static const int KEY_UP    = 265;
static const int KEY_DOWN  = 264;
static const int KEY_LEFT  = 263;
static const int KEY_RIGHT = 262;
static const int KEY_ENTER = 257;
static const int KEY_ESCAPE = 256;

// ---- Stub functions (drawing/audio — no-ops on server) ----

inline void DrawTexture(Texture2D, int, int, Color) {}
inline void DrawTexturePro(Texture2D, Rectangle, Rectangle, Vector2, float, Color) {}
inline void DrawRectangle(int, int, int, int, Color) {}
inline void DrawRectangleV(Vector2, Vector2, Color) {}
inline void DrawRectangleLines(int, int, int, int, Color) {}
inline void DrawCircleV(Vector2, float, Color) {}
inline void DrawRing(Vector2, float, float, float, float, int, Color) {}
inline void DrawText(const char*, int, int, int, Color) {}
inline void TraceLog(int, const char*, ...) {}
inline void InitAudioDevice() {}
inline void CloseAudioDevice() {}
inline Texture2D LoadTexture(const char*) { return {}; }
inline void UnloadTexture(Texture2D) {}
inline Sound LoadSound(const char*) { return {}; }
inline void UnloadSound(Sound) {}
inline void PlaySound(Sound) {}
inline bool IsKeyDown(int) { return false; }
inline bool IsKeyPressed(int) { return false; }
inline bool IsKeyUp(int) { return false; }
inline double GetTime() { return 0.0; }
inline int MeasureText(const char*, int) { return 0; }

inline Color ColorAlpha(Color c, float) { return c; }

// ---- Real implementations (collision — needed by server game logic) ----

inline bool CheckCollisionRecs(Rectangle r1, Rectangle r2) {
    return r1.x < r2.x + r2.width  && r1.x + r1.width  > r2.x &&
           r1.y < r2.y + r2.height && r1.y + r1.height > r2.y;
}

inline Rectangle GetCollisionRec(Rectangle r1, Rectangle r2) {
    float x  = r1.x > r2.x ? r1.x : r2.x;
    float y  = r1.y > r2.y ? r1.y : r2.y;
    float x2 = (r1.x + r1.width)  < (r2.x + r2.width)  ? (r1.x + r1.width)  : (r2.x + r2.width);
    float y2 = (r1.y + r1.height) < (r2.y + r2.height) ? (r1.y + r1.height) : (r2.y + r2.height);
    if (x2 <= x || y2 <= y) return { 0, 0, 0, 0 };
    return { x, y, x2 - x, y2 - y };
}
