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
    bool detachShape(physx::PxShape* shape) override;
    void destroy() override;

    void move(const physx::PxVec3* movement) override;
    void move_to(const physx::PxVec3* target_pos) override;
    void teleport(const physx::PxVec3* target_pos) override;
    void faceTo(const physx::PxVec3* target_pos) override;
    void factTo(physx::PxQuat* rotation) override;

    physx::PxController* controller() const { return m_controller; }

private:
    void refreshFilterData() override;
    float m_radius;
    float m_height;
    physx::PxExtendedVec3 m_position;
    physx::PxController* m_controller;
};

#endif //PHYSXAPI_SCENECHARACTEROBJ_H

