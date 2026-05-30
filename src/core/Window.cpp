#include "Window.h"

Window::Window() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_TITLE);
    SetTargetFPS(60);
}

Window::~Window() {
    CloseWindow();
}

bool Window::ShouldClose() const {
    return WindowShouldClose();
}

void Window::BeginFrame() {
    BeginDrawing();
    ClearBackground(BLACK);
}

void Window::EndFrame() {
    EndDrawing();
}
