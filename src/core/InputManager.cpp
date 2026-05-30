#include "InputManager.h"

bool InputManager::IsKeyDown(int key) {
    return ::IsKeyDown(key);
}

bool InputManager::IsKeyPressed(int key) {
    return ::IsKeyPressed(key);
}

bool InputManager::IsKeyUp(int key) {
    return ::IsKeyUp(key);
}
