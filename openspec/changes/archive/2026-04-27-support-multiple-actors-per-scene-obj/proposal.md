## Why

当前 `SceneObj` 在概念上基本对应一个 PhysX actor，这会限制一个游戏对象同时拥有移动碰撞体、攻击盒、受击盒、触发体等多种物理表示。角色对象已经需要额外的 follower actor 来承载附加 shape，因此这应当成为 `SceneObj` 的显式能力，而不是只存在于 `SceneCharacterObj` 内部的特殊实现。

## What Changes

- 允许一个 `SceneObj` 拥有并管理多个 PhysX actor，并且这些 actor 都能通过 `userData` 解析回同一个 `SceneObj`。
- 引入 actor role，让调用方和内部逻辑可以区分主碰撞/移动 actor、攻击盒、受击盒、触发体以及其它附属 actor。
- 将 `SceneObj` 上的 shape/filter 操作迁移到对应 actor/actor record；`SceneObj` 不再提供隐含单 actor 的 `create*Shape`、`detachShape`、filter refresh、`layer/collideMask/objectFlags` 等入口或状态。
- 移除 `SceneTriggerObj` 作为独立对象类型；trigger 应作为 actor/shape 的接触或查询角色挂在按运动方式划分的 `SceneObj` 子类上。
- 将 `SceneObj` 拥有的所有 actor 的注册身份、生命周期协调、加入/移出 scene 等行为集中管理。
- 定义附属 actor 的姿态同步规则，使移动、传送、旋转更新后，各类 actor 都能与逻辑对象保持一致。
- 本变更暂不考虑 CCD。

## Capabilities

### New Capabilities
- `scene-obj-multiple-actors`: `SceneObj` 可以用多个 PhysX actor 表示同一个逻辑对象，并分别承担碰撞、命中检测、触发等职责。

### Modified Capabilities
- 无。

## Impact

- 影响代码：`SceneObj`、`SceneCharacterObj`、`SceneKinematicObj`、`SceneTriggerObj`、`Scene::createTriggerObject()`，以及所有假设一个 `SceneObj` 只拥有一个 PhysX actor 的 scene/contact/query 代码。
- 影响 API：`SceneObj` 上与 shape/filter 直接相关的入口需要迁移到 actor/actor record；暴露单 actor 或不指定 actor role 的对象 API 需要补充 multi-actor 版本；独立创建 trigger object 的 API 需要迁移为在已有对象上添加 trigger actor/shape。
- 依赖：不引入新的第三方依赖，继续使用现有 PhysX actor、shape、filter、scene API。
- 非目标：CCD 行为、完整战斗玩法规则、JNI 调用过程和对外 JNI API 设计、持久化/序列化变更不在本提案范围内。
