## 1. SceneObj Actor Registry

- [x] 1.1 为 `SceneObj` 增加 actor role enum，覆盖 primary、collider、hitbox、hurtbox、trigger、custom/unknown 等角色。
- [x] 1.2 在 `SceneObj` 中增加 protected actor record 集合，记录 actor pointer、role、local pose、ownership、pose-sync metadata。
- [x] 1.3 实现 protected helper，用于注册/反注册 actor，并确保 registered actor 将 `userData` 设置为 owning `SceneObj`。
- [x] 1.4 实现共享遍历 helper，支持遍历所有 actor、按 role 遍历 actor，以及查找 primary actor。
- [x] 1.5 实现共享销毁逻辑，确保 owned actor 从 `PxScene` 移除并且只 release 一次。

## 2. Filter 和 Shape 处理

- [x] 2.1 重构 filter refresh：从 `SceneObj` 移除全局 layer/collideMask/objectFlags 状态，由各 actor record 持有自己的 filter config 并刷新其 shape 的 simulation/query filter data。
- [x] 2.2 将 `createBoxShape`、`createSphereShape`、`createCapsuleShape` 从 `SceneObj` 迁移到目标 actor/actor record 或底层 shape factory，避免 `SceneObj` 暗含 shape 归属。
- [x] 2.3 将 `detachShape` 从 `SceneObj` 迁移为 role/actor record 定向操作，不再由 `SceneObj` 统一遍历并管理所有 actor 的 shape。
- [x] 2.4 增加 role-aware attach helper，用于将 simulation、hitbox/hurtbox、trigger shape 交给目标 actor record，且不启用 CCD。
- [x] 2.5 增加指定 actor record 的 create/attach/detach helper，shape ownership 和 release 由目标 actor record 负责。
- [x] 2.6 保留现有 shape ownership 行为：attach 成功后释放本地 shape 引用，attach 失败时 ownership 仍归调用方。
- [x] 2.7 将 `setCollisionFilter`、`layer`、`collideMask`、`objectFlags` 相关 API 从 `SceneObj` 迁移到 actor record 或 actor filter config。
- [x] 2.8 确保 hitbox/hurtbox 使用 trigger shape/filter 行为，但不新增 `PHYSXAPI_OBJECT_FLAG_HITBOX` 或 `PHYSXAPI_OBJECT_FLAG_HURTBOX`。

## 3. 对象类型迁移

- [x] 3.1 重构 `SceneStaticObj`，将现有 actor 注册为 primary actor，同时保留 `actor()`。
- [x] 3.2 重构 `SceneRigidObj`，将现有 actor 注册为 primary actor，同时保留 dynamic/kinematic 行为和 `enableCCD`。
- [x] 3.3 重构 `SceneKinematicObj`，将现有 actor 注册为 primary actor，并保持 movement sweep 行为。
- [x] 3.4 重构 `SceneCharacterObj` follower actors，使用共享 registry 替代私有 follower ownership list。
- [x] 3.5 移除 `SceneTriggerObj` 类，将 trigger shape 设置迁移到 role-aware attach helper。
- [x] 3.6 移除或迁移 `Scene::createTriggerObject()`，改为通过合适运动方式的 `SceneObj` 添加 trigger actor/shape。
- [x] 3.7 更新构建配置和 include，删除 `SceneTriggerObj.cpp/.h` 的编译与引用。

## 4. 姿态同步

- [x] 4.1 增加共享 helper，使用 `logical pose * local pose` 同步启用 pose-sync 的 attached actor。
- [x] 4.2 更新 character 的 `move`、`move_to`、`teleport`、`faceTo`、`factTo` 路径，以同步 registered attached actor。
- [x] 4.3 更新 kinematic/static 的 pose-changing 路径，使这些对象类型拥有 secondary actor 时仍能保持同步。
- [x] 4.4 定义并记录 rigid dynamic object 的行为：当 secondary actor 无法在每个 simulation step 自动跟随时，需要怎样显式同步。

## 5. Query 和 Event

- [x] 5.1 将只忽略单 actor 的 query filtering 替换为 owner-aware filtering，使其可以忽略注册到同一 `SceneObj` 的所有 actor。
- [x] 5.2 更新 kinematic movement sweep，使其忽略 moving object 拥有的每个 actor。
- [x] 5.3 更新 contact 和 trigger callback，对同一 `SceneObj` 拥有的 actor pair 一律 suppress object-vs-itself event。
- [x] 5.4 确保 `QueryResult` 对任意 registered actor 仍返回 owning `SceneObj` handle。
- [x] 5.5 确保 trigger event 对 trigger role actor/shape 返回 owning `SceneObj` handle，而不依赖 `SceneTriggerObj`。

## 6. 验证

- [x] 6.1 新增或更新测试/手动检查，验证销毁 multi-actor object 时不会泄漏或 double release。
- [x] 6.2 新增或更新测试/手动检查，验证 primary 和 attached actor record 各自持有 filter config，并只刷新自己管理的 filter data。
- [x] 6.3 新增或更新测试/手动检查，验证 character hitbox/trigger actor 会跟随移动、传送和旋转。
- [x] 6.4 新增或更新测试/手动检查，验证 movement query 和 callback 中 same-owner actor suppression。
- [x] 6.5 新增或更新测试/手动检查，验证不再创建 `SceneTriggerObj` 时仍能创建静态 trigger 和运动 trigger。
- [x] 6.6 构建 native project，并运行可用 test target 或已记录的 smoke scenario。
