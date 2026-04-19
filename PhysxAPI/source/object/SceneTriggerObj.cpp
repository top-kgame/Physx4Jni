//
// Created by zhangakai on 2026/4/17.
//

#include "SceneTriggerObj.h"

#include "../PhysxApi.h"
#include "../scene/Scene.h"

#include <cmath>
#include <vector>

SceneTriggerObj::SceneTriggerObj(Scene* owner)
    : SceneObj(owner),
      m_actor(nullptr)
{
    setObjectFlags(PHYSXAPI_OBJECT_FLAG_TRIGGER);
}

SceneTriggerObj::~SceneTriggerObj()
{
    destroy();
}

bool SceneTriggerObj::initialize()
{
    auto* physics = getPxPhysics();
    if (!physics || !m_owner_scene) return false;
    m_actor = physics->createRigidStatic(physx::PxTransform(physx::PxIdentity));
    if (!m_actor) return false;
    m_actor->userData = this;
    m_owner_scene->pxScene()->addActor(*m_actor);
    return true;
}

void SceneTriggerObj::attachTriggerShape(physx::PxShape* shape)
{
    if (!m_actor || !shape) return;
    applyFilterDataToShape(shape);
    shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
    shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
    m_actor->attachShape(*shape);
    // attach 后释放本地引用，避免泄漏
    shape->release();
}

bool SceneTriggerObj::detachShape(physx::PxShape* shape)
{
    if (!shape) return false;
    if (m_actor)
    {
        m_actor->detachShape(*shape);
    }
    shape->release();
    return true;
}

void SceneTriggerObj::destroy()
{
    if (!m_actor) return;
    if (m_owner_scene && m_owner_scene->pxScene())
    {
        m_owner_scene->pxScene()->removeActor(*m_actor);
    }
    m_actor->release();
    m_actor = nullptr;
}

void SceneTriggerObj::refreshFilterData()
{
    if (!m_actor) return;
    const physx::PxU32 shapeCount = m_actor->getNbShapes();
    if (shapeCount == 0) return;

    std::vector<physx::PxShape*> shapes(shapeCount, nullptr);
    m_actor->getShapes(shapes.data(), shapeCount);
    for (physx::PxShape* shape : shapes)
    {
        applyFilterDataToShape(shape);
    }
}

void SceneTriggerObj::move(const physx::PxVec3* movement)
{
    if (!m_actor || !movement) return;
    auto pose = m_actor->getGlobalPose();
    pose.p += *movement;
    m_actor->setGlobalPose(pose);
}

void SceneTriggerObj::move_to(const physx::PxVec3* target_pos)
{
    teleport(target_pos);
}

void SceneTriggerObj::teleport(const physx::PxVec3* target_pos)
{
    if (!m_actor || !target_pos) return;
    auto pose = m_actor->getGlobalPose();
    pose.p = *target_pos;
    m_actor->setGlobalPose(pose);
}

void SceneTriggerObj::faceTo(const physx::PxVec3* target_pos)
{
    if (!m_actor || !target_pos) return;
    auto pose = m_actor->getGlobalPose();
    const physx::PxVec3 dir = (*target_pos - pose.p);
    if (dir.magnitudeSquared() < 1e-6f) return;
    const physx::PxVec3 f = dir.getNormalized();
    const float yaw = std::atan2f(f.x, f.z);
    pose.q = physx::PxQuat(yaw, physx::PxVec3(0, 1, 0));
    m_actor->setGlobalPose(pose);
}

void SceneTriggerObj::factTo(physx::PxQuat* rotation)
{
    if (!m_actor || !rotation) return;
    auto pose = m_actor->getGlobalPose();
    pose.q = *rotation;
    m_actor->setGlobalPose(pose);
}

