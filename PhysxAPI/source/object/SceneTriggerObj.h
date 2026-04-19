//
// Created by zhangakai on 2026/4/17.
//

#ifndef PHYSXAPI_SCENETRIGGEROBJ_H
#define PHYSXAPI_SCENETRIGGEROBJ_H

#include "SceneObj.h"

class SceneTriggerObj : public SceneObj
{
public:
    explicit SceneTriggerObj(Scene* owner);
    ~SceneTriggerObj() override;

    bool initialize();
    void attachTriggerShape(physx::PxShape* shape);
    bool detachShape(physx::PxShape* shape) override;
    void destroy() override;

    void move(const physx::PxVec3* movement) override;
    void move_to(const physx::PxVec3* target_pos) override;
    void teleport(const physx::PxVec3* target_pos) override;
    void faceTo(const physx::PxVec3* target_pos) override;
    void factTo(physx::PxQuat* rotation) override;

    physx::PxRigidActor* actor() const { return m_actor; }

private:
    void refreshFilterData() override;
    physx::PxRigidStatic* m_actor;
};

#endif //PHYSXAPI_SCENETRIGGEROBJ_H

