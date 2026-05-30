# ecs/ — Entity-Component 系统

通用实体-组件框架，不包含游戏特定逻辑。

## 架构

```
Entity (拥有多个 Component)
  ├── TransformComponent   位置、旋转
  ├── SpriteComponent      纹理渲染
  ├── ColliderComponent    AABB 碰撞检测
  ├── HealthComponent      生命值管理
  └── MovementComponent    移动方向/速度
```

| 文件 | 职责 |
|---|---|
| `Component.h` | 组件抽象基类 |
| `Entity.h/.cpp` | 实体：组件容器 + 类型安全获取 (dynamic_cast) |
| `EntityManager.h/.cpp` | 实体生命周期：创建、更新、渲染、销毁 |
| `components/` | 5 种具体组件 |

## 使用

```cpp
auto* entity = entityManager.CreateEntity<Entity>();
entity->AddComponent<TransformComponent>(position);
entity->AddComponent<ColliderComponent>(layer, size);

auto* health = entity->GetComponent<HealthComponent>();
if (health) health->TakeDamage(1);
```
