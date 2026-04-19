//
// Created by zhangakai on 2026/4/13.
//

#ifndef PHYSXAPI_SCENESTATICOBJ_H
#define PHYSXAPI_SCENESTATICOBJ_H
#include "SceneObj.h"

class SceneStaticObj : public SceneObj
{
public:
    explicit SceneStaticObj(Scene* owner);
    ~SceneStaticObj() override;

    bool initialize();
    void attachShape(physx::PxShape* shape);
    bool detachShape(physx::PxShape* shape) override;
    void destroy() override;

    void move(const physx::PxVec3 *movement) override;
    void move_to(const physx::PxVec3 *target_pos) override;
    void teleport(const physx::PxVec3 *target_pos) override;
    void faceTo(const physx::PxVec3 *target_pos) override;
    void factTo(physx::PxQuat *rotation) override;

    physx::PxRigidStatic* actor() const { return m_actor; }
private:
    void refreshFilterData() override;
    physx::PxRigidStatic* m_actor;
};


#endif //PHYSXAPI_SCENESTATICOBJ_H