// 平台 socket 头文件仅在此 .cpp 中包含, 不污染头文件
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using socket_t = SOCKET;
  static constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  using socket_t = int;
  static constexpr socket_t INVALID_SOCK = -1;
#endif

#include "NetworkManager.h"
#include "NetworkOwner.h"
#include <cstring>
#include <algorithm>

// NetworkDeleter 实现 (在此 .cpp 中定义, 确保 NetworkManager 是完整类型)
void NetworkDeleter::operator()(NetworkManager* p) const { delete p; }

// ============================================================
//  PIMPL: 平台 socket 实现
// ============================================================
struct NetworkManager::Impl {
    socket_t tcpSocket  = INVALID_SOCK;
    socket_t udpSocket  = INVALID_SOCK;
    bool     isHost     = false;
    sockaddr_in udpRemote{};

    ~Impl() { CloseAll(); }

    void CloseAll() {
        CloseSock(tcpSocket);
        CloseSock(udpSocket);
    }

    static void CloseSock(socket_t& s) {
        if (s != INVALID_SOCK) {
#ifdef _WIN32
            closesocket(s);
#else
            ::close(s);
#endif
            s = INVALID_SOCK;
        }
    }

    static bool SetNonBlocking(socket_t s) {
#ifdef _WIN32
        u_long mode = 1;
        return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
        int flags = fcntl(s, F_GETFL, 0);
        return fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
    }

    static bool SetTCPNoDelay(socket_t s) {
        int flag = 1;
        return setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                          reinterpret_cast<const char*>(&flag), sizeof(flag)) == 0;
    }

    static bool SetReuseAddr(socket_t s) {
        int flag = 1;
        return setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                          reinterpret_cast<const char*>(&flag), sizeof(flag)) == 0;
    }
};

// ============================================================
//  Winsock 初始化
// ============================================================
#ifdef _WIN32
static bool g_winsockInit = false;
static void InitSockets() {
    if (!g_winsockInit) {
        WSADATA wsa;
        g_winsockInit = (WSAStartup(MAKEWORD(2, 2), &wsa) == 0);
    }
}
static void CleanupSockets() {
    if (g_winsockInit) { WSACleanup(); g_winsockInit = false; }
}
#else
static void InitSockets() {}
static void CleanupSockets() {}
#endif

NetworkManager::NetworkManager() : impl_(std::make_unique<Impl>()) {
    InitSockets();
    recvBuffer_.reserve(NET_MAX_PACKET_SIZE * 4);
}

NetworkManager::~NetworkManager() {
    Close();
    CloseUDP();
    CleanupSockets();
}

bool NetworkManager::IsConnected() const {
    return impl_->tcpSocket != INVALID_SOCK;
}

// ============================================================
//  Host: Listen / Accept
// ============================================================
bool NetworkManager::Listen(uint16_t port) {
    Close();
    impl_->tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (impl_->tcpSocket == INVALID_SOCK) return false;
    Impl::SetReuseAddr(impl_->tcpSocket);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(impl_->tcpSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        Close();
        return false;
    }
    if (listen(impl_->tcpSocket, 4) != 0) {
        Close();
        return false;
    }
    Impl::SetNonBlocking(impl_->tcpSocket);
    listening_ = true;
    impl_->isHost = true;
    return true;
}

int NetworkManager::AcceptClient() {
    if (!listening_) return -1;
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);
    socket_t clientSock = accept(impl_->tcpSocket,
        reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
    if (clientSock == INVALID_SOCK) return -1;
    Impl::SetTCPNoDelay(clientSock);
    Impl::SetNonBlocking(clientSock);

    ConnectedClient cc;
    cc.socket    = static_cast<int>(clientSock);
    cc.slotIndex = static_cast<int>(clients_.size());
    cc.ready     = false;
    cc.ip        = clientAddr.sin_addr.s_addr;
    cc.port      = clientAddr.sin_port;
    clients_.push_back(cc);
    return cc.socket;
}

void NetworkManager::DisconnectClient(int clientSocket) {
    auto it = std::remove_if(clients_.begin(), clients_.end(),
        [clientSocket](const ConnectedClient& c) { return c.socket == clientSocket; });
    if (it != clients_.end()) {
        socket_t s = static_cast<socket_t>(it->socket);
        Impl::CloseSock(s);
        clients_.erase(it, clients_.end());
    }
}

void NetworkManager::DisconnectAllClients() {
    for (auto& c : clients_) {
        socket_t s = static_cast<socket_t>(c.socket);
        Impl::CloseSock(s);
    }
    clients_.clear();
}

void NetworkManager::SendTo(int clientSocket, const NetMessage& msg) {
    auto data = NetSerializer::Serialize(msg);
    send(static_cast<socket_t>(clientSocket),
         reinterpret_cast<const char*>(data.data()),
         static_cast<int>(data.size()), 0);
}

void NetworkManager::BroadcastTCP(const NetMessage& msg) {
    for (auto& c : clients_) {
        SendTo(c.socket, msg);
    }
}

int NetworkManager::FindClientBySocket(int sock) const {
    for (std::size_t i = 0; i < clients_.size(); i++) {
        if (clients_[i].socket == sock) return static_cast<int>(i);
    }
    return -1;
}

// ============================================================
//  Client
// ============================================================
bool NetworkManager::Connect(const std::string& ip, uint16_t port) {
    Close();
    impl_->tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (impl_->tcpSocket == INVALID_SOCK) return false;
    Impl::SetTCPNoDelay(impl_->tcpSocket);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(impl_->tcpSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        Close();
        return false;
    }
    Impl::SetNonBlocking(impl_->tcpSocket);
    impl_->isHost = false;
    return true;
}

void NetworkManager::Disconnect() { Close(); }

// ============================================================
//  TCP
// ============================================================
bool NetworkManager::Send(const NetMessage& msg) {
    if (impl_->tcpSocket == INVALID_SOCK) return false;
    auto data = NetSerializer::Serialize(msg);
    int sent = send(impl_->tcpSocket, reinterpret_cast<const char*>(data.data()),
                    static_cast<int>(data.size()), 0);
    return sent == static_cast<int>(data.size());
}

bool NetworkManager::Receive(NetMessage& outMsg) {
    if (impl_->tcpSocket == INVALID_SOCK) return false;

    if (recvBuffer_.size() >= 4 + NetMessageHeader::SIZE) {
        uint32_t msgSize;
        std::memcpy(&msgSize, recvBuffer_.data(), 4);
        if (recvBuffer_.size() >= 4 + msgSize) {
            auto [msg, consumed] = NetSerializer::Deserialize(
                recvBuffer_.data(), recvBuffer_.size());
            if (consumed > 0) {
                outMsg = std::move(msg);
                recvBuffer_.erase(recvBuffer_.begin(),
                                  recvBuffer_.begin() + consumed);
                return true;
            }
        }
    }

    uint8_t tmp[2048];
    int n = recv(impl_->tcpSocket, reinterpret_cast<char*>(tmp), sizeof(tmp), 0);
    if (n <= 0) {
#ifdef _WIN32
        if (n < 0 && WSAGetLastError() == WSAEWOULDBLOCK) return false;
#else
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return false;
#endif
        return false;
    }
    recvBuffer_.insert(recvBuffer_.end(), tmp, tmp + n);

    if (recvBuffer_.size() >= 4 + NetMessageHeader::SIZE) {
        uint32_t msgSize;
        std::memcpy(&msgSize, recvBuffer_.data(), 4);
        if (recvBuffer_.size() >= 4 + msgSize) {
            auto [msg, consumed] = NetSerializer::Deserialize(
                recvBuffer_.data(), recvBuffer_.size());
            if (consumed > 0) {
                outMsg = std::move(msg);
                recvBuffer_.erase(recvBuffer_.begin(),
                                  recvBuffer_.begin() + consumed);
                return true;
            }
        }
    }
    return false;
}

// ============================================================
//  Per-client receive (Host 用)
// ============================================================
bool NetworkManager::ReceiveFromClient(int clientSocket, NetMessage& outMsg) {
    socket_t sock = static_cast<socket_t>(clientSocket);
    if (sock == INVALID_SOCK) return false;

    uint8_t tmp[NET_MAX_PACKET_SIZE];
    int n = recv(sock, reinterpret_cast<char*>(tmp), sizeof(tmp), 0);
    if (n <= 0) return false;

    auto [msg, consumed] = NetSerializer::Deserialize(tmp, static_cast<std::size_t>(n));
    if (consumed > 0) {
        outMsg = std::move(msg);
        return true;
    }
    return false;
}

// ============================================================
//  UDP
// ============================================================
bool NetworkManager::BindUDP(uint16_t port) {
    CloseUDP();
    impl_->udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (impl_->udpSocket == INVALID_SOCK) return false;
    Impl::SetReuseAddr(impl_->udpSocket);
    Impl::SetNonBlocking(impl_->udpSocket);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(impl_->udpSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        CloseUDP();
        return false;
    }
    return true;
}

void NetworkManager::SetUDPRemote(const std::string& ip, uint16_t port) {
    std::memset(&impl_->udpRemote, 0, sizeof(impl_->udpRemote));
    impl_->udpRemote.sin_family = AF_INET;
    impl_->udpRemote.sin_port   = htons(port);
    inet_pton(AF_INET, ip.c_str(), &impl_->udpRemote.sin_addr);
}

bool NetworkManager::SendUDP(const NetMessage& msg) {
    if (impl_->udpSocket == INVALID_SOCK) return false;
    auto data = NetSerializer::Serialize(msg);
    int sent = sendto(impl_->udpSocket,
        reinterpret_cast<const char*>(data.data()),
        static_cast<int>(data.size()), 0,
        reinterpret_cast<sockaddr*>(&impl_->udpRemote),
        sizeof(impl_->udpRemote));
    return sent == static_cast<int>(data.size());
}

bool NetworkManager::ReceiveUDP(NetMessage& outMsg) {
    if (impl_->udpSocket == INVALID_SOCK) return false;
    uint8_t tmp[NET_MAX_PACKET_SIZE];
    sockaddr_in from{};
    socklen_t fromLen = sizeof(from);
    int n = recvfrom(impl_->udpSocket,
        reinterpret_cast<char*>(tmp), sizeof(tmp), 0,
        reinterpret_cast<sockaddr*>(&from), &fromLen);
    if (n <= 0) return false;

    lastUDPSenderIP_   = from.sin_addr.s_addr;
    lastUDPSenderPort_ = from.sin_port;

    auto [msg, consumed] = NetSerializer::Deserialize(tmp, static_cast<std::size_t>(n));
    if (consumed > 0) {
        outMsg = std::move(msg);
        return true;
    }
    return false;
}

void NetworkManager::CloseUDP() { Impl::CloseSock(impl_->udpSocket); }

// ============================================================
//  Utilities
// ============================================================
std::string NetworkManager::GetLocalIP() const {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) return "127.0.0.1";

    struct addrinfo hints{}, *result = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0 || !result)
        return "127.0.0.1";

    char ip[INET_ADDRSTRLEN];
    auto* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    freeaddrinfo(result);
    return std::string(ip);
}

void NetworkManager::Close() {
    if (listening_) {
        DisconnectAllClients();
    }
    Impl::CloseSock(impl_->tcpSocket);
    listening_ = false;
    recvBuffer_.clear();
}
