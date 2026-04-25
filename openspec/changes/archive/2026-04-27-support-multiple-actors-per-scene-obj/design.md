## Context

`SceneObj` 是 contact、trigger、query 和 controller hit callback 使用的逻辑对象身份。目前多数对象类型都在派生类成员里保存一个 PhysX actor，并设置 `actor->userData = this`；回调再从 actor 中取回对象 handle。当 static、rigid、kinematic 等对象只需要一个 actor 时，这个模型可以工作。

角色对象已经需要额外的 follower actor 来表示攻击盒/trigger，因为 PhysX controller actor 无法干净地承载所有玩法体积。这部分逻辑当前只存在于 `SceneCharacterObj` 内部，并重复承担了本应抽象出来的职责：actor 注册、`userData` 设置、各 actor 自己的 shape/filter 管理、从 scene 移除，以及附属 actor 姿态同步。

`SceneTriggerObj` 当前作为独立 `SceneObj` 子类存在，但 trigger 本质上描述的是接触/查询方式，而不是对象的运动方式。`SceneObj` 子类应按运动或驱动方式划分，例如 static、rigid、kinematic、character；trigger 应成为这些对象可拥有的 actor role 或 shape flag。

`SceneObj` 不应继续维护任何单 actor 假设。现有放在 `SceneObj` 上的 shape/filter 相关入口和状态，例如 `createBoxShape`、`createSphereShape`、`createCapsuleShape`、`detachShape`、`refreshFilterData`、`layer`、`collideMask`、`objectFlags`，应迁移到对应 actor/actor record 操作上。multi-actor 后，shape 创建、attach/detach、FilterData 和 filter config 应由目标 actor record 管理；`SceneObj` 只负责对象身份、actor registry 和生命周期协调。

## Goals / Non-Goals

**Goals:**
- 用多个 PhysX actor 表示同一个逻辑 `SceneObj`。
- 让每个被拥有的 actor 在 contact、trigger、query、controller hit callback 中都解析到同一个 `SceneObj` handle。
- 记录 actor role，例如 primary、collider、hitbox、hurtbox、trigger，使 shape 行为和玩法解释可以区分。
- 移除 `SceneObj` 上的 shape/filter 操作职责和全局 filter 状态，将 shape 创建、attach/detach、FilterData、layer/collideMask/objectFlags 管理迁移到对应 actor/actor record。
- 移除 `SceneTriggerObj` 这一独立运动对象类型，将 trigger 能力迁移为任意合适 `SceneObj` 子类可添加的 actor/shape role。
- 集中管理共享的 actor registry、identity 和生命周期协调，同时允许各 actor record 管理自己的 shape/filter，派生类保留各自的移动语义。
- 保留现有单 actor 行为，作为最简单的使用方式。

**Non-Goals:**
- CCD 支持或 CCD 策略变更。
- 攻击盒/受击盒的完整战斗与伤害规则。
- JNI 调用过程和对外 JNI API 设计。
- 序列化或持久化数据格式变更。
- 替换 PhysX controller 行为。

## Decisions

1. 增加由 `SceneObj` 拥有的 actor registry。

   `SceneObj` 将拥有一个小型 actor record 集合，每条记录包含：
   - `physx::PxRigidActor* actor`
   - role enum，例如 `PRIMARY`、`COLLIDER`、`HITBOX`、`HURTBOX`、`TRIGGER`
   - 附属/follower actor 相对于逻辑对象的 local pose
   - dynamic、static、controller-backed 对象所需的 ownership/sync 标记

   `SceneObj` 会提供 protected helper，用于注册、反注册、遍历和销毁 owned actor。注册时 MUST 设置 `actor->userData = this`。销毁时，如果对象拥有该 actor，MUST 先从 `PxScene` 移除每个已注册 actor，再 release。shape attach/detach 和 FilterData 刷新不由 `SceneObj` 直接遍历 shape 统一完成，而是路由到目标 actor record，由 actor record 自己处理。

   备选方案：继续在每个派生类里维护独立 follower list。这样可以避免修改基类，但会重复生命周期、filter、query identity 代码，并让代码库中存在多套“actor 属于某个 `SceneObj`”的定义。

2. 移动语义保留在派生类中。

   `SceneObj` 不应决定一次移动使用 controller `move`、rigid body velocity、kinematic target，还是 static pose update。派生类继续按现有方式更新自己的 primary actor/controller，然后调用共享 helper，根据 base transform 同步附属 actor。

   不要为所有 actor 增加基类虚拟移动 helper。这样会模糊 dynamic simulation、kinematic movement、static teleport、controller movement 之间的差异。

3. 将 primary actor 作为兼容现有行为的访问入口。

   对于已有 primary actor 的对象类型，现有 `actor()` accessor 可以继续返回 primary actor。需要关注攻击盒、受击盒、trigger 或所有 owned actor 的新代码，应使用 multi-actor API。`SceneCharacterObj` 的 controller actor 如果可用，可以注册为 primary/collider actor，同时 controller 访问仍保持独立。

   备选方案：立即移除或替换单 `actor()` accessor。这会造成更大的 API 破坏，对第一步 multi-actor 支持来说没有必要。

4. 将 shape/filter 操作迁移到 actor/actor record。

   `SceneObj` 不再提供 shape factory 或 shape detach 入口。box、sphere、capsule 等 shape 创建应由目标 actor/actor record 的 API 完成，或由更底层的 shape factory 完成后显式交给 actor record attach。attach/detach 必须发生在明确的 actor role 或 actor record 上，避免 `SceneObj` 暗含 primary actor 或在多个 actor 中替调用方猜测目标。

   Filter refresh 应迁移为 actor record 自管理：`SceneObj` 不再保存对象级 layer、collideMask、objectFlags。每个 actor record 持有自己的 filter config，并决定如何把这些数据应用到自己的 shape，以及如何处理 role-specific shape flag。如果需要默认 filter 配置，应作为创建 actor record 时的参数或模板，而不是 `SceneObj` 的全局权威状态。

   备选方案：由 `SceneObj` 直接统一遍历所有 actor 的 shape，并实现全局 attach/detach/filter refresh。这样会让基类过度理解 actor 内部 shape 细节，职责过宽，因此不采用。

5. role-specific shape 设置保持显式。

   Filter data 应由各 actor record 应用到自己管理的 shape。shape flag 仍应在 attach 时按 role 显式设置：trigger actor 设置 trigger shape flag，collision/hitbox actor 使用符合其职责的 simulation/query flag，CCD flag 保持不变。

   备选方案：每次 filter refresh 时根据 role 推断 shape flag。这可能覆盖调用方控制的 shape 配置，并让 filter refresh 修改 filter 以外的状态。

6. 必须显式考虑同一对象内部 actor 的交互。

   multi-actor 对象会带来同一个 `SceneObj` 的多个 actor 互相碰撞、overlap 或 query hit 的可能。movement sweep 和 gameplay query 应能忽略移动/查询对象拥有的所有 actor，而不只是单个 actor。contact/trigger event 代码应避免发出 object-vs-itself 事件，除非明确需要。

   备选方案：像现在一样只忽略 primary actor。这会让附属攻击盒/trigger 对 owner 自身可见，从而产生错误的移动阻挡或 self event。

7. 移除 `SceneTriggerObj`，把 trigger 建模为 role。

   `SceneTriggerObj` 的职责应拆分：其“静态不动”的部分属于 static/kinematic 等运动方式；其“触发回调”的部分属于 trigger actor role 和 shape flag。实现上应删除 `SceneTriggerObj` 类和专用创建路径，提供在已有 `SceneObj` 上添加 trigger actor/shape 的能力；如果仍需要世界空间固定 trigger，可以用 static/kinematic 对象承载 trigger role。

   备选方案：保留 `SceneTriggerObj` 并同时增加 trigger role。这样会让 trigger 同时作为对象类型和 actor role 存在，概念重复，并继续鼓励按接触方式划分 `SceneObj` 子类。

8. hitbox/hurtbox 不新增 object flag。

   hitbox 和 hurtbox 都是 trigger 体积，不参与推挤、阻挡或 solver，也不是 primary actor。底层 PhysX filter 只需要把它们识别为 trigger，因此继续使用现有 `PHYSXAPI_OBJECT_FLAG_TRIGGER` 或 shape 的 `eTRIGGER_SHAPE` 即可。`HITBOX`、`HURTBOX` 保留为 actor role 或 shape metadata，用于 C++ event/gameplay 层解释 trigger overlap 的含义。

   备选方案：新增 `PHYSXAPI_OBJECT_FLAG_HITBOX` 和 `PHYSXAPI_OBJECT_FLAG_HURTBOX`。这会消耗 filter bit，并把 gameplay 语义提前下沉到 PhysX filter 层；在当前需求中没有必要。

9. same-owner actor 事件默认全部 suppress。

   如果 contact 或 trigger pair 的两个 actor 都属于同一个 `SceneObj`，系统不应进入正式事件流。这样可以避免对象自己的 hitbox、hurtbox、trigger 或 collider 互相产生 self event，例如角色自己的 hitbox 打到自己的 hurtbox。调试重叠关系应通过 debug draw、debug query 或专门工具完成，而不是通过正式 contact/trigger event 例外上报。

   备选方案：允许某些 role pair（例如 hitbox-vs-hurtbox）上报 self event。当前没有玩法需求，且会让事件语义更复杂，因此不采用。

## Risks / Trade-offs

- 同一 `SceneObj` 拥有的 actor 之间发生 self-collision/self-trigger -> 在 movement/query 路径增加 owner-aware filtering 或 query ignore list，并在 contact/trigger callback 中 suppress same-owner event。
- 基类职责变宽 -> 将 `SceneObj` 限定在 registry、ownership、identity、iteration 和 pose sync 协调；shape/filter config 和 FilterData 细节交给 actor record，移动和玩法解释仍留在派生类。
- 现有外部代码可能直接使用 `actor()` -> 保留 primary actor accessor，并新增 API，而不是在本变更中移除它们。
- 现有 shape API 调用方可能依赖 `SceneObj::create*Shape` 或 `SceneObj::detachShape` -> 将调用迁移到明确的 actor/actor record API；兼容旧接口时只允许作为短期 wrapper 并内部转发，不在 `SceneObj` 中集中维护 shape 状态。
- 现有外部代码可能使用 `Scene::createTriggerObject()` -> 提供明确迁移路径，将其替换为创建合适运动类型对象后添加 trigger actor/shape；必要时短期保留兼容 wrapper，但内部不再依赖 `SceneTriggerObj`。
- 带附属 role actor 的 dynamic actor 如果只在 teleport 时同步 follower，可能发生漂移 -> 按对象类型定义 `move`、`move_to`、`teleport` 和旋转变化的准确同步点。
- shape/actor release 错误可能导致 double-release 或泄漏 -> 保持当前 ownership 约定：成功 attach 已创建 shape 后释放本地 shape 引用；除非标记为 non-owning，否则 actor registry 负责 release 已注册 actor。
- hitbox/hurtbox 只作为 role 可能无法在 PhysX filter shader 中直接区分 -> 当前不需要按 hitbox/hurtbox 做 solver/filter 分支；如未来需要，可再评估是否增加 filter flag。

## Migration Plan

1. 增加 `SceneObj` actor registry 和 protected helper。
2. 迁移 `SceneObj` shape/filter API 和状态：移除或转发 `create*Shape`、`detachShape`、`refreshFilterData`、`setCollisionFilter`、`layer/collideMask/objectFlags` 等入口，新增 actor/actor record 定向 shape/filter 操作，并让 shape/filter config 由 actor record 自管理。
3. 重构现有单 actor 对象，将当前 actor 注册为 primary actor，同时保留现有 public accessor。
4. 将 `SceneCharacterObj` follower actor 的 ownership/filter/sync 逻辑迁移到共享 registry。
5. 根据当前对象类型需要，增加 hitbox、hurtbox、trigger、collider role 的 multi-actor attach helper。
6. 移除 `SceneTriggerObj` 类和专用创建路径，将现有 trigger object 用法迁移到 static/kinematic 等运动对象上的 trigger role。
7. 更新当前只忽略一个 actor 的 movement/query 路径，使其可以忽略同一 `SceneObj` 拥有的所有 actor。
8. 通过聚焦测试/手动检查验证 contact、trigger、query、destroy 和 filter update。

在 API consumer 采用新 helper 之前，回滚较直接：撤销 registry 重构，保留每个派生类自己的 actor 成员。一旦外部开始使用新的 role API，回滚就需要把这些调用替换回之前的单 actor attach 行为。

## Open Questions

- 无。
