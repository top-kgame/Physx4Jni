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
    destroy();
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

void SceneKinematicObj::makeDynamic(float density)
{
    if (!m_actor) return;
    m_actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
    if (density > 0.0f)
    {
        physx::PxRigidBodyExt::updateMassAndInertia(*m_actor, density);
    }
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
class IgnoreSelfQueryFilter final : public physx::PxQueryFilterCallback
{
public:
    IgnoreSelfQueryFilter(const physx::PxRigidActor* self, uint32_t queryMask)
        : m_self(self), m_query_mask(queryMask) {}

    physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData&,
                                          const physx::PxShape* shape,
                                          const physx::PxRigidActor* actor,
                                          physx::PxHitFlags&) override
    {
        if (actor == m_self) return physx::PxQueryHitType::eNONE;
        if (!shape) return physx::PxQueryHitType::eNONE;
        const physx::PxFilterData shapeFilterData = shape->getQueryFilterData();
        if ((shapeFilterData.word0 & m_query_mask) == 0) return physx::PxQueryHitType::eNONE;
        return physx::PxQueryHitType::eBLOCK;
    }

    physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData&,
                                           const physx::PxQueryHit&,
                                           const physx::PxShape*,
                                           const physx::PxRigidActor*) override
    {
        return physx::PxQueryHitType::eBLOCK;
    }

private:
    const physx::PxRigidActor* m_self;
    uint32_t m_query_mask;
};
} // namespace

void SceneKinematicObj::move(const physx::PxVec3* movement)
{
    if (!m_actor || !movement || !m_owner_scene) return;

    const float dist = movement->magnitude();
    if (dist < 1e-6f) return;

    physx::PxTransform pose = m_actor->getGlobalPose();

    physx::PxShape* shapes[1] = {nullptr};
    const physx::PxU32 shapeCount = m_actor->getShapes(shapes, 1);
    if (shapeCount == 0 || !shapes[0] || !m_owner_scene->pxScene())
    {
        pose.p += *movement;
        m_actor->setKinematicTarget(pose);
        return;
    }

    const physx::PxVec3 dir = (*movement) / dist;
    physx::PxGeometryHolder geomHolder = shapes[0]->getGeometry();

    physx::PxSweepBuffer hit;
    physx::PxQueryFilterData qfd;
    qfd.flags = physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER;

    IgnoreSelfQueryFilter filter(m_actor, collideMask());
    const bool hasHit = m_owner_scene->pxScene()->sweep(geomHolder.any(), pose, dir, dist, hit,
                                                        physx::PxHitFlag::eDEFAULT, qfd, &filter);

    physx::PxVec3 finalMove = *movement;
    if (hasHit && hit.hasBlock)
    {
        // 轻量 skin，避免贴面造成抖动/穿插
        constexpr float skin = 0.001f;
        const float allowed = physx::PxMax(0.0f, hit.block.distance - skin);
        finalMove = dir * allowed;
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
    pose.q = physx::PxQuat(yaw, PhysxApiWorldUp());
    m_actor->setGlobalPose(pose);
}

void SceneKinematicObj::factTo(physx::PxQuat* rotation)
{
    if (!m_actor || !rotation) return;
    auto pose = m_actor->getGlobalPose();
    pose.q = *rotation;
    m_actor->setGlobalPose(pose);
}

