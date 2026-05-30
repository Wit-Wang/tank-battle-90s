# game/ — 游戏逻辑层

Tank Battle 90s 的所有游戏规则和实体行为。

## tank/ — 坦克系统

组合优于继承：一个 `Tank` 类 + `TankType` 枚举 + `IController` 策略接口。

| 文件 | 职责 |
|---|---|
| `Tank.h/.cpp` | 坦克核心：Entity + Controller 组合，开火/受伤/无敌 |
| `TankType.h` | 4 种类型枚举 + `TankStats` 属性表 (数据驱动) |
| `TankFactory.h/.cpp` | 工厂模式：创建 Player/AI 坦克 |
| `controller/IController.h` | 控制器策略接口 |
| `controller/PlayerController.h/.cpp` | 键盘输入控制器 |
| `controller/AIController.h/.cpp` | AI 状态机 (PATROL→CHASE→ATTACK) |
| `controller/NetController.h/.cpp` | 网络控制器 (远程玩家) |

### 属性表

| 类型 | 速度 | 子弹速度 | 伤害 | 生命 | 护甲 | 冷却 | 子弹 |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| LIGHT | 100 | 350 | 1 | 1 | 0 | 0.5s | 2 |
| MEDIUM | 80 | 300 | 2 | 2 | 1 | 0.7s | 1 |
| HEAVY | 55 | 250 | 2 | 3 | 2 | 1.0s | 1 |
| SPEED | 130 | 400 | 1 | 1 | 0 | 0.3s | 3 |

## entities/ — 游戏实体

| 文件 | 职责 |
|---|---|
| `Bullet.h/.cpp` | 子弹：方向移动 + 生命周期 |
| `Wall.h/.cpp` | 墙壁：砖墙 (可破坏) / 钢墙 (不可破坏) |
| `Base.h/.cpp` | 基地：被摧毁则判定胜负 |
| `PowerUp.h/.cpp` | 道具：STAR/BOMB/SHOVEL/TANK/HELMET |

## map/ — 地图系统

| 文件 | 职责 |
|---|---|
| `TileType.h` | 瓦片类型枚举 |
| `GameMap.h/.cpp` | 13x13 瓦片网格：加载、渲染、碰撞查询 |
| `MapManager.h/.cpp` | 按模式扫描/选择地图 |

## systems/ — 游戏系统

| 文件 | 职责 |
|---|---|
| `CollisionSystem.h/.cpp` | 子弹-地图、坦克-地图 (重叠推力)、实体-实体 |
| `ParticleSystem.h/.cpp` | 粒子特效：爆炸、击中 |

## session/ — 会话管理

| 文件 | 职责 |
|---|---|
| `GameSession.h/.cpp` | 4 槽位会话：模式、地图、AI 随机化、队伍平衡 |
| `PlayerSlot.h` | 槽位：人/AI、坦克类型、队伍、键位、重生状态 |
