//
// Created by zhangakai on 2026/4/13.
//

#include "SceneObj.h"

#include "../PhysxApi.h"
#include "../scene/Scene.h"

#include <algorithm>

SceneObj::SceneObj(Scene* owner)
    : m_owner_scene(owner),
      m_handle(reinterpret_cast<uint64_t>(this))
{
}

physx::PxShape* SceneObj::ActorRecord::createBoxShape(const physx::PxVec3& halfExtents) const
{
    auto* physics = getPxPhysics();
    const auto* mat = getDefaultMaterial();
    if (!physics || !mat) return nullptr;
    return physics->createShape(physx::PxBoxGeometry(halfExtents), *mat, true);
}

physx::PxShape* SceneObj::ActorRecord::createSphereShape(float radius) const
{
    auto* physics = getPxPhysics();
    auto* mat = getDefaultMaterial();
    if (!physics || !mat) return nullptr;
    return physics->createShape(physx::PxSphereGeometry(radius), *mat, true);
}

physx::PxShape* SceneObj::ActorRecord::createCapsuleShape(float radius, float halfHeight) const
{
    auto* physics = getPxPhysics();
    auto* mat = getDefaultMaterial();
    if (!physics || !mat) return nullptr;
    return physics->createShape(physx::PxCapsuleGeometry(radius, halfHeight), *mat, true);
}

void SceneObj::ActorRecord::setCollisionFilter(uint32_t layer, uint32_t collideMask)
{
    filter.layer = layer;
    filter.collideMask = collideMask;
    refreshFilterData();
}

void SceneObj::ActorRecord::setObjectFlags(uint32_t objectFlags)
{
    filter.objectFlags = objectFlags;
    refreshFilterData();
}

physx::PxFilterData SceneObj::ActorRecord::simulationFilterData() const
{
    return MakePhysxApiSimulationFilterData(filter.layer, filter.collideMask, filter.objectFlags);
}

physx::PxFilterData SceneObj::ActorRecord::queryFilterData() const
{
    return MakePhysxApiQueryShapeFilterData(filter.layer, filter.objectFlags);
}

void SceneObj::ActorRecord::applyFilterDataToShape(physx::PxShape* shape) const
{
    if (!shape) return;
    shape->setSimulationFilterData(simulationFilterData());
    shape->setQueryFilterData(queryFilterData());
}

void SceneObj::ActorRecord::refreshFilterData() const
{
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

bool SceneObj::ActorRecord::attachShape(physx::PxShape* shape)
{
    if (!actor || !shape) return false;

    applyFilterDataToShape(shape);
    const bool triggerRole = role == ActorRole::TRIGGER || role == ActorRole::HITBOX || role == ActorRole::HURTBOX;
    if (triggerRole)
    {
        shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
        shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
    }
    else
    {
        shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, false);
    }

    if (!actor->attachShape(*shape)) return false;
    if (updateMassOnShapeChange)
    {
        if (auto* dynamicActor = actor->is<physx::PxRigidDynamic>())
        {
            physx::PxRigidBodyExt::updateMassAndInertia(*dynamicActor, density);
        }
    }

    // createShape 返回的 shape 由调用方持有一次引用；attach 成功后释放本地引用，失败时仍归调用方。
    shape->release();
    return true;
}

bool SceneObj::ActorRecord::detachShape(physx::PxShape* shape)
{
    if (!actor || !shape) return false;

    const physx::PxU32 shapeCount = actor->getNbShapes();
    std::vector<physx::PxShape*> shapes(shapeCount, nullptr);
    actor->getShapes(shapes.data(), shapeCount);
    const bool isAttached = std::find(shapes.begin(), shapes.end(), shape) != shapes.end();
    if (!isAttached) return false;

    actor->detachShape(*shape);
    shape->release();
    if (updateMassOnShapeChange)
    {
        if (auto* dynamicActor = actor->is<physx::PxRigidDynamic>())
        {
            physx::PxRigidBodyExt::updateMassAndInertia(*dynamicActor, density);
        }
    }
    return true;
}

SceneObj::ActorRecord* SceneObj::actorRecord(ActorRole role)
{
    for (ActorRecord& record : m_actor_records)
    {
        if (record.role == role) return &record;
    }
    return nullptr;
}

const SceneObj::ActorRecord* SceneObj::actorRecord(ActorRole role) const
{
    for (const ActorRecord& record : m_actor_records)
    {
        if (record.role == role) return &record;
    }
    return nullptr;
}

physx::PxRigidActor* SceneObj::primaryActor() const
{
    const ActorRecord* record = primaryActorRecord();
    return record ? record->actor : nullptr;
}

bool SceneObj::ownsActor(const physx::PxActor* actor) const
{
    if (!actor) return false;
    for (const ActorRecord& record : m_actor_records)
    {
        if (record.actor == actor) return true;
    }
    return false;
}

void SceneObj::forEachActor(const std::function<void(const ActorRecord&)>& visitor) const
{
    if (!visitor) return;
    for (const ActorRecord& record : m_actor_records)
    {
        visitor(record);
    }
}

void SceneObj::forEachActor(ActorRole role, const std::function<void(const ActorRecord&)>& visitor) const
{
    if (!visitor) return;
    for (const ActorRecord& record : m_actor_records)
    {
        if (record.role == role) visitor(record);
    }
}

bool SceneObj::attachShape(ActorRole role, physx::PxShape* shape)
{
    ActorRecord* record = actorRecord(role);
    return record ? record->attachShape(shape) : false;
}

bool SceneObj::detachShape(ActorRole role, physx::PxShape* shape)
{
    ActorRecord* record = actorRecord(role);
    return record ? record->detachShape(shape) : false;
}

bool SceneObj::addHitboxShape(physx::PxShape* shape, const physx::PxTransform& localPose)
{
    ActorFilterConfig filter;
    filter.objectFlags = PHYSXAPI_OBJECT_FLAG_TRIGGER;
    return addAttachedShape(ActorRole::HITBOX, shape, localPose, filter);
}

bool SceneObj::addHurtboxShape(physx::PxShape* shape, const physx::PxTransform& localPose)
{
    ActorFilterConfig filter;
    filter.objectFlags = PHYSXAPI_OBJECT_FLAG_TRIGGER;
    return addAttachedShape(ActorRole::HURTBOX, shape, localPose, filter);
}

bool SceneObj::addTriggerShape(physx::PxShape* shape, const physx::PxTransform& localPose)
{
    ActorFilterConfig filter;
    filter.objectFlags = PHYSXAPI_OBJECT_FLAG_TRIGGER;
    return addAttachedShape(ActorRole::TRIGGER, shape, localPose, filter);
}

void SceneObj::syncAttachedActorsToLogicalPose()
{
    syncAttachedActorsPose(logicalPose());
}

SceneObj::ActorRecord* SceneObj::registerActor(physx::PxRigidActor* actor,
                                               ActorRole role,
                                               const physx::PxTransform& localPose,
                                               bool owned,
                                               bool syncPose,
                                               const ActorFilterConfig& filter,
                                               bool updateMassOnShapeChange,
                                               float density)
{
    if (!actor) return nullptr;
    actor->userData = this;

    ActorRecord record;
    record.owner = this;
    record.actor = actor;
    record.role = role;
    record.localPose = localPose;
    record.owned = owned;
    record.syncPose = syncPose;
    record.updateMassOnShapeChange = updateMassOnShapeChange;
    record.density = density;
    record.filter = filter;
    m_actor_records.push_back(record);
    return &m_actor_records.back();
}

bool SceneObj::unregisterActor(physx::PxRigidActor* actor, bool releaseOwnedActor)
{
    if (!actor) return false;
    for (auto it = m_actor_records.begin(); it != m_actor_records.end(); ++it)
    {
        if (it->actor != actor) continue;
        if (actor->userData == this) actor->userData = nullptr;
        if (releaseOwnedActor && it->owned)
        {
            if (m_owner_scene && m_owner_scene->pxScene())
            {
                m_owner_scene->pxScene()->removeActor(*actor);
            }
            actor->release();
        }
        m_actor_records.erase(it);
        return true;
    }
    return false;
}

void SceneObj::destroyRegisteredActors()
{
    for (auto it = m_actor_records.rbegin(); it != m_actor_records.rend(); ++it)
    {
        physx::PxRigidActor* actor = it->actor;
        if (!actor) continue;
        if (actor->userData == this) actor->userData = nullptr;
        if (!it->owned) continue;

        if (m_owner_scene && m_owner_scene->pxScene())
        {
            m_owner_scene->pxScene()->removeActor(*actor);
        }
        actor->release();
    }
    m_actor_records.clear();
}

SceneObj::ActorRecord* SceneObj::createAttachedKinematicActor(ActorRole role,
                                                              const physx::PxTransform& localPose,
                                                              const ActorFilterConfig& filter)
{
    auto* physics = getPxPhysics();
    if (!physics || !m_owner_scene || !m_owner_scene->pxScene()) return nullptr;

    physx::PxRigidDynamic* actor = physics->createRigidDynamic(logicalPose() * localPose);
    if (!actor) return nullptr;

    actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
    actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
    m_owner_scene->pxScene()->addActor(*actor);
    return registerActor(actor, role, localPose, true, true, filter);
}

bool SceneObj::addAttachedShape(ActorRole role,
                                physx::PxShape* shape,
                                const physx::PxTransform& localPose,
                                const ActorFilterConfig& filter)
{
    if (!shape) return false;
    ActorRecord* record = createAttachedKinematicActor(role, localPose, filter);
    if (!record) return false;
    if (!record->attachShape(shape))
    {
        unregisterActor(record->actor, true);
        return false;
    }
    syncAttachedActorsPose(logicalPose());
    return true;
}

void SceneObj::syncAttachedActorsPose(const physx::PxTransform& logicalPose)
{
    for (ActorRecord& record : m_actor_records)
    {
        if (!record.actor || !record.syncPose) continue;
        const physx::PxTransform pose = logicalPose * record.localPose;
        if (auto* dynamicActor = record.actor->is<physx::PxRigidDynamic>())
        {
            if (dynamicActor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)
            {
                dynamicActor->setKinematicTarget(pose);
            }
            else
            {
                dynamicActor->setGlobalPose(pose);
            }
        }
        else
        {
            record.actor->setGlobalPose(pose);
        }
    }
}
