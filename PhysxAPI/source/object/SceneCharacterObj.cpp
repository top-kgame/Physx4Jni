//
// Created by zhangakai on 2026/4/17.
//

#include "SceneCharacterObj.h"

#include "../PhysxApi.h"
#include "../scene/Scene.h"

#include <cmath>
#include <vector>

SceneCharacterObj::SceneCharacterObj(Scene* owner, float radius, float height, const physx::PxExtendedVec3& position)
    : SceneObj(owner),
      m_radius(radius),
      m_height(height),
      m_position(position),
      m_controller(nullptr)
{
    setObjectFlags(PHYSXAPI_OBJECT_FLAG_CHARACTER);
}

SceneCharacterObj::~SceneCharacterObj()
{
    SceneCharacterObj::destroy();
}

bool SceneCharacterObj::initialize()
{
    auto* physics = getPxPhysics();
    if (!physics || !m_owner_scene) return false;
    auto* mgr = m_owner_scene->controllerManager();
    if (!mgr) return false;

    physx::PxCapsuleControllerDesc desc;
    desc.radius = m_radius;
    desc.height = m_height;
    desc.material = getDefaultMaterial();
    desc.position = m_position;
    desc.upDirection = m_owner_scene->characterControllerUpDirection();
    desc.slopeLimit = m_owner_scene->characterControllerSlopeLimit();
    desc.stepOffset = m_owner_scene->characterControllerStepOffset();
    desc.contactOffset = m_owner_scene->characterControllerContactOffset();
    desc.reportCallback = m_owner_scene->controllerHitReport();

    if (!desc.isValid()) return false;

    m_controller = mgr->createController(desc);
    if (m_controller)
    {
        m_controller->setUserData(this);
        if (auto* actor = m_controller->getActor())
        {
            actor->userData = this;
        }
    }
    refreshFilterData();
    return m_controller != nullptr;
}

bool SceneCharacterObj::detachShape(physx::PxShape* shape)
{
    // CCT 不提供 shape attach/detach 的封装入口（controller 内部形状不对外暴露）
    (void)shape;
    return false;
}

void SceneCharacterObj::destroy()
{
    if (!m_controller) return;
    m_controller->release();
    m_controller = nullptr;
}

void SceneCharacterObj::refreshFilterData()
{
    if (!m_controller) return;
    physx::PxRigidDynamic* actor = m_controller->getActor();
    if (!actor) return;

    const physx::PxU32 shapeCount = actor->getNbShapes();
    if (shapeCount == 0) return;

    std::vector<physx::PxShape*> shapes(shapeCount, nullptr);
    actor->getShapes(shapes.data(), shapeCount);
    for (physx::PxShape* shape : shapes)
    {
        applyFilterDataToShape(shape);
    }
}

void SceneCharacterObj::move(const physx::PxVec3* movement)
{
    if (!m_controller || !movement) return;
    physx::PxControllerFilters filters;
    const float minDist = m_owner_scene ? m_owner_scene->characterControllerMinMoveDistance() : 0.001f;
    const float dt = m_owner_scene ? m_owner_scene->fixedDeltaTime() : (1.0f / 60.0f);
    m_controller->move(*movement, minDist, dt, filters);
}

void SceneCharacterObj::move_to(const physx::PxVec3* target_pos)
{
    teleport(target_pos);
}

void SceneCharacterObj::teleport(const physx::PxVec3* target_pos)
{
    if (!m_controller || !target_pos) return;
    m_controller->setPosition(physx::PxExtendedVec3(target_pos->x, target_pos->y, target_pos->z));
}

void SceneCharacterObj::faceTo(const physx::PxVec3* target_pos)
{
    // CCT 本身没有强制朝向；这里留空作为最小实现（由上层角色逻辑维护朝向）
    (void)target_pos;
}

void SceneCharacterObj::factTo(physx::PxQuat* rotation)
{
    (void)rotation;
}

