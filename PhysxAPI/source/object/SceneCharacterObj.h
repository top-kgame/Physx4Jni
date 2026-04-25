//
// Created by zhangakai on 2026/4/17.
//

#ifndef PHYSXAPI_SCENECHARACTEROBJ_H
#define PHYSXAPI_SCENECHARACTEROBJ_H

#include "SceneObj.h"

class SceneCharacterObj : public SceneObj
{
public:
    SceneCharacterObj(Scene* owner, float radius, float height, const physx::PxExtendedVec3& position);
    ~SceneCharacterObj() override;

    bool initialize();
    void destroy() override;

    void move(const physx::PxVec3* movement) override;
    void move_to(const physx::PxVec3* target_pos) override;
    void teleport(const physx::PxVec3* target_pos) override;
    void faceTo(const physx::PxVec3* target_pos) override;
    void factTo(physx::PxQuat* rotation) override;

    physx::PxController* controller() const { return m_controller; }

    /**
     * 为角色添加一个跟随的 hitbox（simulation shape）actor，并挂载 shape。
     * localPose 以角色为原点（CCT position）定义。
     * shape 在成功 attach 后会 release 本地引用（与其它 SceneObj 风格一致）。
     * @return 创建/挂载成功返回 true，否则 false（失败时不接管 shape 引用）
     */
    bool addHitboxShape(physx::PxShape* shape, const physx::PxTransform& localPose = physx::PxTransform(physx::PxIdentity));

    /**
     * 为角色添加一个跟随的 trigger actor，并挂载 trigger shape。
     * shape 在成功 attach 后会 release 本地引用（与其它 SceneObj 风格一致）。
     */
    bool addTriggerShape(physx::PxShape* shape, const physx::PxTransform& localPose = physx::PxTransform(physx::PxIdentity));

private:
    physx::PxTransform logicalPose() const override;

    float m_radius;
    float m_height;
    physx::PxExtendedVec3 m_position;
    physx::PxQuat m_rotation;
    physx::PxController* m_controller;
};

#endif //PHYSXAPI_SCENECHARACTEROBJ_H

