# net/ — 网络层

局域网联机，**Host-Authoritative** 模型 + **PIMPL** 编译隔离。

## 文件

| 文件 | 职责 |
|---|---|
| `NetProtocol.h` | 消息协议：19 种消息类型、序列化、二进制数据结构 |
| `NetworkManager.h/.cpp` | PIMPL 网络管理器 (TCP)，socket 头文件仅在 .cpp |
| `NetworkOwner.h` | `NetworkManagerPtr` 类型 (前向声明删除器) |
| `NetController.h/.cpp` | 网络控制器：实现 IController，由网络输入驱动 |

## PIMPL 隔离

Winsock2 与 raylib 的 `Rectangle`、`DrawText` 冲突。通过 PIMPL 将 socket 类型隐藏在 `NetworkManager::Impl` 中，其他文件无需包含 socket 头文件。

## 消息协议

```
消息头 (8 字节): [Magic 4B "TB90"] [Type 2B] [Length 2B]
消息体: 变长二进制数据
```

| 类别 | 消息 |
|---|---|
| 连接 | JoinRequest/Accepted/Rejected, PlayerJoined/Left, Kick, Ping/Pong |
| 大厅 | LobbyState, SetMap/Team/TankType, ClientReady |
| 游戏 | GameStart, GameState (20Hz), InputState (60Hz), WallChanged, GameOver |

## 常量

| 常量 | 值 |
|---|---|
| NET_DEFAULT_PORT | 7777 |
| NET_MAX_PACKET_SIZE | 4096 |
| NET_STATE_TICK_INTERVAL | 50ms (20Hz) |
| NET_INPUT_TICK_INTERVAL | 16ms (~60Hz) |
