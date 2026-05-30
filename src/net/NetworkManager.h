#pragma once

#include "NetProtocol.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

/// 连接的客户端信息 (不依赖 socket 头文件)
struct ConnectedClient {
    int    socket    = -1;     // socket fd (int)
    int    slotIndex = -1;
    bool   ready     = false;
    float  lastPing  = 0.f;
    uint32_t ip      = 0;     // 网络字节序
    uint16_t port    = 0;     // 网络字节序
};

/// 网络管理器：TCP 连接 + UDP 通信
/// 使用 PIMPL 隐藏所有平台 socket 类型, 避免 Windows 头文件污染
class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    // ---- Host ----
    bool Listen(uint16_t port = NET_DEFAULT_PORT);
    int  AcceptClient();
    void DisconnectClient(int clientSocket);
    void DisconnectAllClients();

    void SendTo(int clientSocket, const NetMessage& msg);
    void BroadcastTCP(const NetMessage& msg);

    const std::vector<ConnectedClient>& GetClients() const { return clients_; }
    std::vector<ConnectedClient>& GetClients() { return clients_; }
    int  FindClientBySocket(int sock) const;
    int  ClientCount() const { return static_cast<int>(clients_.size()); }

    // ---- Client ----
    bool Connect(const std::string& ip, uint16_t port = NET_DEFAULT_PORT);
    void Disconnect();

    // ---- TCP ----
    bool Send(const NetMessage& msg);
    bool Receive(NetMessage& outMsg);
    /// 从指定客户端 socket 接收消息 (非阻塞)
    bool ReceiveFromClient(int clientSocket, NetMessage& outMsg);
    bool IsConnected() const;
    bool IsListening() const { return listening_; }

    // ---- UDP ----
    bool BindUDP(uint16_t port);
    void SetUDPRemote(const std::string& ip, uint16_t port);
    bool SendUDP(const NetMessage& msg);
    bool ReceiveUDP(NetMessage& outMsg);
    /// 最近一次 ReceiveUDP 的来源 IP (网络字节序)
    uint32_t GetLastUDPSenderIP() const { return lastUDPSenderIP_; }
    /// 最近一次 ReceiveUDP 的来源端口 (网络字节序)
    uint16_t GetLastUDPSenderPort() const { return lastUDPSenderPort_; }
    void CloseUDP();

    // ---- 工具 ----
    std::string GetLocalIP() const;
    void Close();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    bool listening_ = false;
    std::vector<ConnectedClient> clients_;
    std::vector<uint8_t>         recvBuffer_;

    uint32_t lastUDPSenderIP_   = 0;
    uint16_t lastUDPSenderPort_ = 0;
};
