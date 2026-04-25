//
// Created by zhangakai on 2026/4/17.
//

#include "SceneKinematicObj.h"

#include "../PhysxApi.h"
#include "../scene/Scene.h"

#include <PxPhysicsAPI.h>

#include <cmath>

SceneKinematicObj::SceneKinematicObj(Scene* owner)
    : SceneObj(owner),
      m_actor(nullptr)
{
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
    ActorFilterConfig filter;
    filter.objectFlags = PHYSXAPI_OBJECT_FLAG_KINEMATIC;
    registerActor(m_actor, ActorRole::PRIMARY, physx::PxTransform(physx::PxIdentity), true, false, filter);
    m_owner_scene->pxScene()->addActor(*m_actor);
    return true;
}

void SceneKinematicObj::attachShape(physx::PxShape* shape)
{
    SceneObj::attachShape(ActorRole::PRIMARY, shape);
}

bool SceneKinematicObj::detachShape(physx::PxShape* shape)
{
    return SceneObj::detachShape(ActorRole::PRIMARY, shape);
}

void SceneKinematicObj::destroy()
{
    if (!m_actor) return;
    destroyRegisteredActors();
    m_actor = nullptr;
}

void SceneKinematicObj::makeDynamic(float density)
{
    if (!m_actor) return;
    m_actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
    if (density > 0.0f)
    {
        physx::PxRigidBodyExt::updateMassAndInertia(*m_actor, density);
    }
}

physx::PxTransform SceneKinematicObj::logicalPose() const
{
    return m_actor ? m_actor->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
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
                const ActorRecord* primary = primaryActorRecord();
                const uint32_t collideMask = primary ? primary->collideMask() : PHYSXAPI_ALL_LAYERS;
                const QueryResult sweepResult =
                    m_owner_scene->sweep(geomHolder.any(), shapePose, dir, dist, collideMask, this);

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
    syncAttachedActorsPose(pose);
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
    syncAttachedActorsPose(pose);
}

void SceneKinematicObj::faceTo(const physx::PxVec3* target_pos)
{
    if (!m_actor || !target_pos) return;
    auto pose = m_actor->getGlobalPose();
    const physx::PxVec3 dir = (*target_pos - pose.p);
    if (dir.magnitudeSquared() < 1e-6f) return;
    const physx::PxVec3 f = dir.getNormalized();
    const float yaw = std::atan2f(f.x, f.z);
    pose.q = physx::PxQuat(yaw, PhysxApiWorldUp());
    m_actor->setGlobalPose(pose);
    syncAttachedActorsPose(pose);
}

void SceneKinematicObj::factTo(physx::PxQuat* rotation)
{
    if (!m_actor || !rotation) return;
    auto pose = m_actor->getGlobalPose();
    pose.q = *rotation;
    m_actor->setGlobalPose(pose);
    syncAttachedActorsPose(pose);
}

