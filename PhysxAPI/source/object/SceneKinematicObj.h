//
// Created by zhangakai on 2026/4/17.
//

#ifndef PHYSXAPI_SCENEKINEMATICOBJ_H
#define PHYSXAPI_SCENEKINEMATICOBJ_H

#include "SceneObj.h"

class SceneKinematicObj : public SceneObj
{
public:
    explicit SceneKinematicObj(Scene* owner);
    ~SceneKinematicObj() override;

    bool initialize();
    void attachShape(physx::PxShape* shape);
    bool detachShape(physx::PxShape* shape) override;
    void destroy() override;

    void move(const physx::PxVec3* movement) override;
    void move_to(const physx::PxVec3* target_pos) override;
    void teleport(const physx::PxVec3* target_pos) override;
    void faceTo(const physx::PxVec3* target_pos) override;
    void factTo(physx::PxQuat* rotation) override;

    physx::PxRigidDynamic* actor() const { return m_actor; }

private:
    void refreshFilterData() override;
    physx::PxRigidDynamic* m_actor;
};

#endif //PHYSXAPI_SCENEKINEMATICOBJ_H

