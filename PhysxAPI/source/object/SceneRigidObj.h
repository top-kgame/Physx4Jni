//
// Created by zhangakai on 2026/4/17.
//

#ifndef PHYSXAPI_SCENERIGIDOBJ_H
#define PHYSXAPI_SCENERIGIDOBJ_H

#include "SceneObj.h"

class SceneRigidObj : public SceneObj
{
public:
    explicit SceneRigidObj(Scene* owner);
    ~SceneRigidObj() override;

    bool initialize(bool kinematic);
    void attachShape(physx::PxShape* shape);
    bool detachShape(physx::PxShape* shape) override;
    void destroy() override;

    void move(const physx::PxVec3* movement) override;
    void move_to(const physx::PxVec3* target_pos) override;
    void teleport(const physx::PxVec3* target_pos) override;
    void faceTo(const physx::PxVec3* target_pos) override;
    void factTo(physx::PxQuat* rotation) override;

    // CCD（按需）：只在刚体类型上暴露
    void enableCCD(bool enable);

    physx::PxRigidDynamic* actor() const { return m_actor; }

private:
    void refreshFilterData() override;
    physx::PxRigidDynamic* m_actor;
};

#endif //PHYSXAPI_SCENERIGIDOBJ_H

