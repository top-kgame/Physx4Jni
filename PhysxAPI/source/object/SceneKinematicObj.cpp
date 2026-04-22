//
// Created by zhangakai on 2026/4/17.
//

#include "SceneKinematicObj.h"

#include "../PhysxApi.h"
#include "../scene/Scene.h"

#include <PxPhysicsAPI.h>

#include <cmath>
#include <vector>

SceneKinematicObj::SceneKinematicObj(Scene* owner)
    : SceneObj(owner),
      m_actor(nullptr)
{
    setObjectFlags(PHYSXAPI_OBJECT_FLAG_KINEMATIC);
}

SceneKinematicObj::~SceneKinematicObj()
{
    SceneKinematicObj::destroy();
}

bool SceneKinematicObj::initialize()
{
    auto* physics = getPxPhysics();
    if (!physics || !m_owner_scene) return false;

    m_actor = physics->createRigidDynamic(physx::PxTransform(physx::PxIdentity));
    if (!m_actor) return false;

    m_actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
    m_actor->userData = this;
    m_owner_scene->pxScene()->addActor(*m_actor);
    return true;
}

void SceneKinematicObj::attachShape(physx::PxShape* shape)
{
    if (!m_actor || !shape) return;
    applyFilterDataToShape(shape);
    m_actor->attachShape(*shape);
    // kinematic 仍可挂质量属性（不用于动力学求解，但保持数据完整）
    physx::PxRigidBodyExt::updateMassAndInertia(*m_actor, 10.0f);
    // attach 后释放本地引用，避免泄漏
    shape->release();
}

bool SceneKinematicObj::detachShape(physx::PxShape* shape)
{
    if (!shape) return false;
    if (m_actor)
    {
        m_actor->detachShape(*shape);
    }
    shape->release();
    return true;
}

void SceneKinematicObj::destroy()
{
    if (!m_actor) return;
    if (m_owner_scene && m_owner_scene->pxScene())
    {
        m_owner_scene->pxScene()->removeActor(*m_actor);
    }
    m_actor->release();
    m_actor = nullptr;
}

void SceneKinematicObj::refreshFilterData()
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

namespace
{
    constexpr float kKinematicSweepSkin = 0.001f;
} // namespace

void SceneKinematicObj::move(const physx::PxVec3* movement)
{
    if (!m_actor || !movement || !m_owner_scene) return;

    const float dist = movement->magnitude();
    if (dist < 1e-6f) return;

    physx::PxTransform pose = m_actor->getGlobalPose();

    const physx::PxVec3 dir = (*movement) / dist;

    physx::PxVec3 finalMove = *movement;
    if (m_owner_scene->pxScene())
    {
        const physx::PxU32 shapeCount = m_actor->getNbShapes();
        if (shapeCount > 0)
        {
            std::vector<physx::PxShape*> shapes(shapeCount, nullptr);
            m_actor->getShapes(shapes.data(), shapeCount);

            bool hasAnyHit = false;
            float nearestHitDistance = dist;

            for (const physx::PxShape* shape : shapes)
            {
                if (!shape) continue;

                const physx::PxShapeFlags flags = shape->getFlags();
                if (!(flags & physx::PxShapeFlag::eSCENE_QUERY_SHAPE)) continue;
                if (flags & physx::PxShapeFlag::eTRIGGER_SHAPE) continue;

                const physx::PxGeometryHolder geomHolder = shape->getGeometry();
                const physx::PxTransform shapePose = pose * shape->getLocalPose();
                const QueryResult sweepResult =
                    m_owner_scene->sweep(geomHolder.any(), shapePose, dir, dist, collideMask(), m_actor);

                if (sweepResult.hasHit)
                {
                    hasAnyHit = true;
                    nearestHitDistance = physx::PxMin(nearestHitDistance, sweepResult.distance);
                }
            }

            if (hasAnyHit)
            {
                // 轻量 skin，避免贴面造成抖动/穿插
                const float allowed = physx::PxMax(0.0f, nearestHitDistance - kKinematicSweepSkin);
                finalMove = dir * allowed;
            }
        }
    }

    pose.p += finalMove;
    m_actor->setKinematicTarget(pose);
}

void SceneKinematicObj::move_to(const physx::PxVec3* target_pos)
{
    if (!m_actor || !target_pos) return;
    const physx::PxVec3 cur = m_actor->getGlobalPose().p;
    const physx::PxVec3 movement = (*target_pos - cur);
    move(&movement);
}

void SceneKinematicObj::teleport(const physx::PxVec3* target_pos)
{
    if (!m_actor || !target_pos) return;
    auto pose = m_actor->getGlobalPose();
    pose.p = *target_pos;
    m_actor->setGlobalPose(pose);
    m_actor->setLinearVelocity(physx::PxVec3(0));
    m_actor->setAngularVelocity(physx::PxVec3(0));
}

void SceneKinematicObj::faceTo(const physx::PxVec3* target_pos)
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

void SceneKinematicObj::factTo(physx::PxQuat* rotation)
{
    if (!m_actor || !rotation) return;
    auto pose = m_actor->getGlobalPose();
    pose.q = *rotation;
    m_actor->setGlobalPose(pose);
}

