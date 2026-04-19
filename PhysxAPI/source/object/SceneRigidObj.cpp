//
// Created by zhangakai on 2026/4/17.
//

#include "SceneRigidObj.h"

#include "../PhysxApi.h"
#include "../scene/Scene.h"

#include <cmath>
#include <vector>

SceneRigidObj::SceneRigidObj(Scene* owner)
    : SceneObj(owner),
      m_actor(nullptr)
{
}

SceneRigidObj::~SceneRigidObj()
{
    destroy();
}

bool SceneRigidObj::initialize(bool kinematic)
{
    auto* physics = getPxPhysics();
    if (!physics || !m_owner_scene) return false;

    m_actor = physics->createRigidDynamic(physx::PxTransform(physx::PxIdentity));
    if (!m_actor) return false;

    m_actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, kinematic);
    m_actor->userData = this;
    m_owner_scene->pxScene()->addActor(*m_actor);
    return true;
}

void SceneRigidObj::attachShape(physx::PxShape* shape)
{
    if (!m_actor || !shape) return;
    applyFilterDataToShape(shape);
    m_actor->attachShape(*shape);
    physx::PxRigidBodyExt::updateMassAndInertia(*m_actor, 10.0f);
    // attach 后释放本地引用，避免泄漏
    shape->release();
}

bool SceneRigidObj::detachShape(physx::PxShape* shape)
{
    if (!shape) return false;
    if (m_actor)
    {
        m_actor->detachShape(*shape);
    }
    shape->release();
    return true;
}

void SceneRigidObj::destroy()
{
    if (!m_actor) return;
    if (m_owner_scene && m_owner_scene->pxScene())
    {
        m_owner_scene->pxScene()->removeActor(*m_actor);
    }
    m_actor->release();
    m_actor = nullptr;
}

void SceneRigidObj::refreshFilterData()
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

void SceneRigidObj::move(const physx::PxVec3* movement)
{
    if (!m_actor || !movement) return;
    const float dt = m_owner_scene ? m_owner_scene->fixedDeltaTime() : (1.0f / 60.0f);
    if (dt <= 0.0f) return;

    const physx::PxVec3 vel = (*movement) / dt;
    m_actor->setLinearVelocity(vel);
    m_actor->wakeUp();
}

void SceneRigidObj::move_to(const physx::PxVec3* target_pos)
{
    teleport(target_pos);
}

void SceneRigidObj::teleport(const physx::PxVec3* target_pos)
{
    if (!m_actor || !target_pos) return;
    auto pose = m_actor->getGlobalPose();
    pose.p = *target_pos;
    m_actor->setGlobalPose(pose);
    m_actor->setLinearVelocity(physx::PxVec3(0));
    m_actor->setAngularVelocity(physx::PxVec3(0));
}

void SceneRigidObj::faceTo(const physx::PxVec3* target_pos)
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

void SceneRigidObj::factTo(physx::PxQuat* rotation)
{
    if (!m_actor || !rotation) return;
    auto pose = m_actor->getGlobalPose();
    pose.q = *rotation;
    m_actor->setGlobalPose(pose);
}

void SceneRigidObj::enableCCD(bool enable)
{
    if (!m_actor) return;
    m_actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, enable);
}

