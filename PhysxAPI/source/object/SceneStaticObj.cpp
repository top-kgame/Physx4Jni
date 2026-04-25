//
// Created by zhangakai on 2026/4/13.
//

#include "SceneStaticObj.h"

#include "../PhysxApi.h"
#include "../scene/Scene.h"

#include <cmath>

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
    registerActor(m_actor, ActorRole::PRIMARY, physx::PxTransform(physx::PxIdentity), true, false, ActorFilterConfig{});
    m_owner_scene->pxScene()->addActor(*m_actor);
    return true;
}

void SceneStaticObj::attachShape(physx::PxShape* shape)
{
    SceneObj::attachShape(ActorRole::PRIMARY, shape);
}

bool SceneStaticObj::detachShape(physx::PxShape* shape)
{
    return SceneObj::detachShape(ActorRole::PRIMARY, shape);
}

void SceneStaticObj::destroy()
{
    if (!m_actor) return;
    destroyRegisteredActors();
    m_actor = nullptr;
}

physx::PxTransform SceneStaticObj::logicalPose() const
{
    return m_actor ? m_actor->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
}

void SceneStaticObj::move(const physx::PxVec3* movement)
{
    if (!m_actor || !movement) return;
    auto pose = m_actor->getGlobalPose();
    pose.p += *movement;
    m_actor->setGlobalPose(pose);
    syncAttachedActorsPose(pose);
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
    syncAttachedActorsPose(pose);
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
    syncAttachedActorsPose(pose);
}

void SceneStaticObj::factTo(physx::PxQuat* rotation)
{
    if (!m_actor || !rotation) return;
    auto pose = m_actor->getGlobalPose();
    pose.q = *rotation;
    m_actor->setGlobalPose(pose);
    syncAttachedActorsPose(pose);
}