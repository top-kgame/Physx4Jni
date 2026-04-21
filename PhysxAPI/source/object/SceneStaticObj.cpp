//
// Created by zhangakai on 2026/4/13.
//

#include "SceneStaticObj.h"

#include "../PhysxApi.h"
#include "../scene/Scene.h"

#include <cmath>
#include <vector>

SceneStaticObj::SceneStaticObj(Scene* owner)
    : SceneObj(owner),
      m_actor(nullptr)
{
}

SceneStaticObj::~SceneStaticObj()
{
    destroy();
}

bool SceneStaticObj::initialize()
{
    auto* physics = getPxPhysics();
    if (!physics || !m_owner_scene) return false;
    m_actor = physics->createRigidStatic(physx::PxTransform(physx::PxIdentity));
    if (!m_actor) return false;
    m_actor->userData = this;
    m_owner_scene->pxScene()->addActor(*m_actor);
    return true;
}

void SceneStaticObj::attachShape(physx::PxShape* shape)
{
    if (!m_actor || !shape) return;
    applyFilterDataToShape(shape);
    m_actor->attachShape(*shape);
    // createShape 返回的 shape 由调用方持有一次引用；attach 后释放本地引用，避免泄漏
    shape->release();
}

bool SceneStaticObj::detachShape(physx::PxShape* shape)
{
    if (!shape) return false;
    if (m_actor)
    {
        m_actor->detachShape(*shape);
    }
    shape->release();
    return true;
}

void SceneStaticObj::destroy()
{
    if (!m_actor) return;
    if (m_owner_scene && m_owner_scene->pxScene())
    {
        m_owner_scene->pxScene()->removeActor(*m_actor);
    }
    m_actor->release();
    m_actor = nullptr;
}

void SceneStaticObj::refreshFilterData()
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

void SceneStaticObj::move(const physx::PxVec3* movement)
{
    if (!m_actor || !movement) return;
    auto pose = m_actor->getGlobalPose();
    pose.p += *movement;
    m_actor->setGlobalPose(pose);
}

void SceneStaticObj::move_to(const physx::PxVec3* target_pos)
{
    teleport(target_pos);
}

void SceneStaticObj::teleport(const physx::PxVec3* target_pos)
{
    if (!m_actor || !target_pos) return;
    auto pose = m_actor->getGlobalPose();
    pose.p = *target_pos;
    m_actor->setGlobalPose(pose);
}

void SceneStaticObj::faceTo(const physx::PxVec3* target_pos)
{
    if (!m_actor || !target_pos) return;
    auto pose = m_actor->getGlobalPose();
    const physx::PxVec3 dir = (*target_pos - pose.p);
    if (dir.magnitudeSquared() < 1e-6f) return;
    const physx::PxVec3 f = dir.getNormalized();
    // 仅按 Yaw 旋转（Y 轴向上）
    const float yaw = std::atan2f(f.x, f.z);
    pose.q = physx::PxQuat(yaw, PhysxApiWorldUp());
    m_actor->setGlobalPose(pose);
}

void SceneStaticObj::factTo(physx::PxQuat* rotation)
{
    if (!m_actor || !rotation) return;
    auto pose = m_actor->getGlobalPose();
    pose.q = *rotation;
    m_actor->setGlobalPose(pose);
}