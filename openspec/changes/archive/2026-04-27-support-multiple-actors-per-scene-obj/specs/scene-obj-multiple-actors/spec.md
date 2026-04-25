## ADDED Requirements

### Requirement: SceneObj 拥有多个 PhysX actor
系统 SHALL 允许单个 `SceneObj` 注册并拥有多个 PhysX rigid actor，同时保持一个逻辑对象 handle。

#### Scenario: 多个 actor 解析为同一个对象
- **WHEN** contact、trigger 或 scene query 上报任意一个注册到 `SceneObj` 的 PhysX actor
- **THEN** 上报的 object handle MUST 是 owning `SceneObj` 的 handle

#### Scenario: Destroy 释放所有 owned actor
- **WHEN** 拥有多个 registered actor 的 `SceneObj` 被销毁
- **THEN** 每个 owned registered actor MUST 从 PhysX scene 移除，并且只 release 一次

### Requirement: Actor role 区分物理职责
系统 SHALL 为每个 registered actor 记录 role，使一个 `SceneObj` 可以区分 primary movement/collision actor、hitbox、hurtbox、trigger 以及其它 attached actor。

#### Scenario: 注册时指定 role
- **WHEN** 一个 actor 注册到 `SceneObj`
- **THEN** 该 actor MUST 带有显式 role 存储

#### Scenario: 不同 role actor 共享对象身份
- **WHEN** callback 或 query 观察到属于同一 `SceneObj` 的不同 actor role
- **THEN** 它们 MUST 仍然识别为同一个 `SceneObj` handle

### Requirement: Trigger 作为 actor/shape role
系统 SHALL 将 trigger 建模为 `SceneObj` 拥有的 actor/shape role，而不是独立的 `SceneObj` 子类。

#### Scenario: 对已有对象添加 trigger
- **WHEN** 调用方需要为一个已有 `SceneObj` 添加 trigger 体积
- **THEN** 系统 MUST 允许在该 `SceneObj` 上注册 trigger actor 或 attach trigger shape

#### Scenario: 不再创建 SceneTriggerObj
- **WHEN** 调用方需要一个静态或运动的 trigger 体积
- **THEN** 系统 MUST 使用合适运动方式的 `SceneObj` 子类承载 trigger role，而不是创建 `SceneTriggerObj`

#### Scenario: Trigger event 使用 owner 身份
- **WHEN** trigger actor 或 trigger shape 产生 trigger event
- **THEN** 事件中的 trigger handle MUST 是 owning `SceneObj` 的 handle

#### Scenario: Hitbox hurtbox 使用 trigger 行为
- **WHEN** 系统创建 hitbox 或 hurtbox actor/shape
- **THEN** 该 actor/shape MUST 使用 trigger 行为，并且 MUST NOT 为 hitbox 或 hurtbox 新增独立 object flag

### Requirement: Actor record 管理自己的 FilterData
系统 SHALL 让每个 registered actor record 管理自己的 filter config，并管理自己 actor 上 shape 的 simulation 和 query filter data。

#### Scenario: Actor filter 变化
- **WHEN** 调用方修改某个 actor record 的 layer、collideMask 或 objectFlags
- **THEN** 该 actor record MUST 刷新自己管理的 shape filter data

#### Scenario: SceneObj 不保存全局 filter state
- **WHEN** 一个 `SceneObj` 拥有多个 registered actor record
- **THEN** `SceneObj` MUST NOT 作为 layer、collideMask 或 objectFlags 的全局权威状态；这些 filter config MUST 存放在 actor record 或其 shape 配置中

### Requirement: Shape API 属于目标 actor record
系统 SHALL 将 shape 创建、attach/detach API 放在目标 actor 或 actor record 上，使调用方明确 shape 作用于哪个 actor role 或 actor record，并由目标 actor record 管理自己的 shape。

#### Scenario: 按 role attach shape
- **WHEN** 调用方将 shape 添加到某个 actor role
- **THEN** 系统 MUST 将操作路由到该 role 对应的 actor record，或在需要时创建并注册该 role 的 actor record

#### Scenario: Detach shape 指定目标 actor
- **WHEN** 调用方 detach 一个 shape
- **THEN** 调用方 MUST 指定目标 actor role 或 actor record，系统 MUST 将 detach 操作路由给该 actor record

#### Scenario: SceneObj 不创建 shape
- **WHEN** 调用方需要创建 box、sphere 或 capsule shape
- **THEN** 系统 MUST 通过目标 actor/actor record 或底层 shape factory 创建 shape，不得通过 `SceneObj` 隐式创建或 attach 到 primary actor

### Requirement: Attached actor 与逻辑对象姿态同步
系统 SHALL 按每个 actor 的 local pose，使 attached actor 与 `SceneObj` 的 logical transform 保持对齐。

#### Scenario: 对象移动
- **WHEN** `SceneObj` 的 movement operation 更新 logical object pose
- **THEN** 每个参与 pose synchronization 的 attached actor MUST 移动到 `logical pose * local pose`

#### Scenario: 对象旋转
- **WHEN** `SceneObj` 的 facing 或 rotation operation 更新 logical object orientation
- **THEN** 每个参与 pose synchronization 的 attached actor MUST 使用更新后的 orientation 并组合自身 local pose

### Requirement: 保留现有 primary actor 行为
系统 SHALL 为已经暴露 primary actor 的对象类型保留现有 single-primary-actor 行为。

#### Scenario: 单 actor 对象
- **WHEN** 现有 static、rigid 或 kinematic object 初始化
- **THEN** 其现有 actor accessor MUST 继续返回 primary PhysX actor

#### Scenario: Multi-actor 对象
- **WHEN** 一个对象拥有额外的 hitbox、hurtbox、trigger 或 collider actor
- **THEN** primary actor access MUST 仍然返回 primary actor，而不是任意 attached actor

### Requirement: 显式处理同 owner actor 交互
系统 SHALL 避免同一个 `SceneObj` 拥有的多个 actor 之间发生意外 self-interaction。

#### Scenario: Movement query 忽略 owner actors
- **WHEN** `SceneObj` 执行一个忽略自身的 movement sweep 或 query
- **THEN** query MUST 忽略注册到该 `SceneObj` 的所有 actor，而不只是 primary actor

#### Scenario: Self contact callback
- **WHEN** PhysX 上报 contact 或 trigger pair，且两个 actor 都属于同一个 `SceneObj`
- **THEN** 系统 MUST suppress object-vs-itself event，不得按 role pair 例外上报到正式事件流

### Requirement: CCD 保持不变
系统 SHALL NOT 在 multiple-actor `SceneObj` 支持中引入 CCD 行为。

#### Scenario: 创建额外 actor
- **WHEN** `SceneObj` 为 hitbox、hurtbox、trigger 或 collider 创建或注册额外 actor
- **THEN** 系统 MUST NOT 作为本变更的一部分为这些 actor 启用 CCD
