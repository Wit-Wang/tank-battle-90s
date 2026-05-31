#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <stdexcept>
#include <type_traits>

// ============================================================
//  网络协议常量
// ============================================================
constexpr uint16_t NET_DEFAULT_PORT      = 4562;
constexpr uint32_t NET_MAGIC             = 0x54423930;  // "TB90"
constexpr uint32_t NET_MAX_PACKET_SIZE   = 4096;
constexpr uint32_t NET_MAX_MESSAGE_SIZE  = NET_MAX_PACKET_SIZE - 8;

constexpr float NET_STATE_TICK_INTERVAL  = 0.05f;   // 20Hz 状态广播
constexpr float NET_INPUT_TICK_INTERVAL  = 0.016f;  // ~60Hz 输入发送
constexpr float NET_TIMEOUT_SECONDS      = 5.0f;
constexpr float NET_PING_INTERVAL        = 2.0f;   // send pings every 2s

// ---- 客户端输入位掩码 ----
constexpr uint8_t NET_INPUT_UP    = 1;
constexpr uint8_t NET_INPUT_DOWN  = 2;
constexpr uint8_t NET_INPUT_LEFT  = 4;
constexpr uint8_t NET_INPUT_RIGHT = 8;
constexpr uint8_t NET_INPUT_FIRE  = 16;

// ============================================================
//  消息类型
// ============================================================
enum class NetMessageType : uint16_t {
    // 连接
    JoinRequest     = 100,
    JoinAccepted    = 101,
    JoinRejected    = 102,
    PlayerJoined    = 103,
    PlayerLeft      = 104,
    Kick            = 105,
    Ping            = 106,
    Pong            = 107,
    // 大厅
    LobbyState      = 200,
    SetMap          = 201,
    SetTeam         = 202,
    SetTankType     = 203,
    ClientReady     = 204,
    // 游戏
    GameStart       = 300,
    GameState       = 301,
    InputState      = 302,
    BulletCreated   = 303,
    BulletDestroyed = 304,
    WallChanged     = 305,
    GameOver        = 306,
    ReturnToLobby   = 307,
};

// ============================================================
//  消息头 (8 字节)
// ============================================================
struct NetMessageHeader {
    uint32_t       magic       = NET_MAGIC;
    NetMessageType type        = NetMessageType::Ping;
    uint16_t       payloadSize = 0;
    static constexpr std::size_t SIZE = 8;
};

// ============================================================
//  消息 (头 + 载荷)
// ============================================================
struct NetMessage {
    NetMessageHeader       header;
    std::vector<uint8_t>   payload;

    NetMessage() = default;
    explicit NetMessage(NetMessageType t) { header.type = t; }

    template<typename T>
    void WritePayload(const T& data) {
        static_assert(std::is_trivially_copyable_v<T>);
        const auto* b = reinterpret_cast<const uint8_t*>(&data);
        payload.insert(payload.end(), b, b + sizeof(T));
        header.payloadSize = static_cast<uint16_t>(payload.size());
    }

    void WriteString(const std::string& str) {
        uint16_t len = static_cast<uint16_t>(str.size());
        const auto* lb = reinterpret_cast<const uint8_t*>(&len);
        payload.insert(payload.end(), lb, lb + 2);
        payload.insert(payload.end(), str.begin(), str.end());
        header.payloadSize = static_cast<uint16_t>(payload.size());
    }

    void WriteBytes(const uint8_t* data, std::size_t sz) {
        payload.insert(payload.end(), data, data + sz);
        header.payloadSize = static_cast<uint16_t>(payload.size());
    }

    template<typename T>
    T ReadPayload(std::size_t offset = 0) const {
        static_assert(std::is_trivially_copyable_v<T>);
        if (offset + sizeof(T) > payload.size())
            throw std::runtime_error("NetMessage: read overflow");
        T result;
        std::memcpy(&result, payload.data() + offset, sizeof(T));
        return result;
    }

    std::string ReadString(std::size_t offset = 0) const {
        if (offset + 2 > payload.size())
            throw std::runtime_error("NetMessage: string len overflow");
        uint16_t len;
        std::memcpy(&len, payload.data() + offset, 2);
        if (offset + 2 + len > payload.size())
            throw std::runtime_error("NetMessage: string data overflow");
        return std::string(
            reinterpret_cast<const char*>(payload.data() + offset + 2), len);
    }
};

// ConnectedClient 定义在 NetworkManager.h 中 (依赖 socket 头文件)

// ============================================================
//  序列化: 消息 <-> 字节流 (4字节长度前缀)
// ============================================================
namespace NetSerializer {

    inline std::vector<uint8_t> Serialize(const NetMessage& msg) {
        std::vector<uint8_t> buf;
        uint32_t totalSize = NetMessageHeader::SIZE + msg.header.payloadSize;
        const auto* sb = reinterpret_cast<const uint8_t*>(&totalSize);
        buf.insert(buf.end(), sb, sb + 4);
        const auto* hb = reinterpret_cast<const uint8_t*>(&msg.header);
        buf.insert(buf.end(), hb, hb + NetMessageHeader::SIZE);
        buf.insert(buf.end(), msg.payload.begin(), msg.payload.end());
        return buf;
    }

    inline std::pair<NetMessage, std::size_t> Deserialize(
        const uint8_t* data, std::size_t size)
    {
        NetMessage msg;
        if (size < 4 + NetMessageHeader::SIZE) return {msg, 0};
        uint32_t msgSize;
        std::memcpy(&msgSize, data, 4);
        if (msgSize > NET_MAX_MESSAGE_SIZE || size < 4 + msgSize) return {msg, 0};
        std::memcpy(&msg.header, data + 4, NetMessageHeader::SIZE);
        if (msg.header.magic != NET_MAGIC) return {msg, 0};
        if (msg.header.payloadSize > 0) {
            if (msg.header.payloadSize > msgSize - NetMessageHeader::SIZE) return {msg, 0};
            msg.payload.resize(msg.header.payloadSize);
            std::memcpy(msg.payload.data(),
                        data + 4 + NetMessageHeader::SIZE,
                        msg.header.payloadSize);
        }
        return {msg, 4 + msgSize};
    }

} // namespace NetSerializer

// ============================================================
//  游戏状态二进制数据结构 (GameState 载荷)
// ============================================================
#pragma pack(push, 1)

struct NetPlayerState {
    float    x, y;
    float    rotation;
    uint8_t  hp;
    uint8_t  maxHp;
    uint8_t  alive;       // 0=dead, 1=alive
    uint8_t  invincible;
};

struct NetBulletState {
    uint8_t  bulletId;
    float    x, y;
    uint8_t  ownerTeam;
    uint8_t  active;
};

struct NetBulletCreatedPayload {
    uint8_t  bulletId;
    float    x, y;
    uint8_t  ownerTeam;
    float    dirX, dirY;
};

struct NetBulletDestroyedPayload {
    uint8_t bulletId;
};

struct NetWallChangedPayload {
    uint8_t  col, row;
    uint8_t  newType;   // TileType
};

struct NetInputPayload {
    uint16_t seq;
    uint8_t  inputMask; // NET_INPUT_UP | ... | NET_INPUT_FIRE
};

#pragma pack(pop)