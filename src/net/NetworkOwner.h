#pragma once

#include <memory>

/// NetworkManager 的前向声明删除器
/// 定义在 NetworkManager.cpp 中, 避免 unique_ptr 要求完整类型
class NetworkManager;
struct NetworkDeleter { void operator()(NetworkManager* p) const; };
using NetworkManagerPtr = std::unique_ptr<NetworkManager, NetworkDeleter>;
