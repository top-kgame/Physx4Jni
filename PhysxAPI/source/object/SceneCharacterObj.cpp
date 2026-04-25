//
// Created by zhangakai on 2026/4/17.
//

#include "SceneCharacterObj.h"

#include "../PhysxApi.h"
#include "../scene/Scene.h"

#include <PxPhysicsAPI.h>

#include <cmath>

namespace
{
constexpr float kDefaultCharacterControllerMinMoveDistance = 0.001f;
} // namespace

SceneCharacterObj::SceneCharacterObj(Scene* owner, float radius, float height, const physx::PxExtendedVec3& position)
    : SceneObj(owner),
      m_radius(radius),
      m_height(height),
      m_position(position),
      m_rotation(physx::PxIdentity),
      m_controller(nullptr)
{
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
            ActorFilterConfig filter;
            filter.objectFlags = PHYSXAPI_OBJECT_FLAG_CHARACTER;
            ActorRecord* record = registerActor(actor, ActorRole::PRIMARY, physx::PxTransform(physx::PxIdentity), false, false, filter);
            if (record) record->refreshFilterData();
        }
    }
    syncAttachedActorsPose(logicalPose());
    return m_controller != nullptr;
}

void SceneCharacterObj::destroy()
{
    destroyRegisteredActors();
    if (m_controller)
    {
        m_controller->release();
        m_controller = nullptr;
    }
}

physx::PxTransform SceneCharacterObj::logicalPose() const
{
    const physx::PxExtendedVec3 cur = m_controller ? m_controller->getPosition() : m_position;
    return physx::PxTransform(physx::PxVec3(static_cast<float>(cur.x),
                                            static_cast<float>(cur.y),
                                            static_cast<float>(cur.z)),
                              m_rotation);
}

void SceneCharacterObj::move(const physx::PxVec3* movement)
{
    if (!m_controller || !movement) return;
    physx::PxControllerFilters filters;
    const float minDist = m_owner_scene ? m_owner_scene->characterControllerMinMoveDistance() : kDefaultCharacterControllerMinMoveDistance;
    const float dt = m_owner_scene ? m_owner_scene->fixedDeltaTime() : Scene::kDefaultFixedDeltaTime;
    m_controller->move(*movement, minDist, dt, filters);
    syncAttachedActorsPose(logicalPose());
}

void SceneCharacterObj::move_to(const physx::PxVec3* target_pos)
{
    if (!m_controller || !target_pos) return;
    const physx::PxExtendedVec3 cur = m_controller->getPosition();
    const physx::PxVec3 movement(target_pos->x - static_cast<float>(cur.x),
                                 target_pos->y - static_cast<float>(cur.y),
                                 target_pos->z - static_cast<float>(cur.z));
    move(&movement);
}

void SceneCharacterObj::teleport(const physx::PxVec3* target_pos)
{
    if (!m_controller || !target_pos) return;
    m_controller->setPosition(physx::PxExtendedVec3(target_pos->x, target_pos->y, target_pos->z));
    syncAttachedActorsPose(logicalPose());
}

void SceneCharacterObj::faceTo(const physx::PxVec3* target_pos)
{
    if (!m_controller || !target_pos) return;
    const physx::PxExtendedVec3 cur = m_controller->getPosition();
    const physx::PxVec3 cur3(static_cast<float>(cur.x), static_cast<float>(cur.y), static_cast<float>(cur.z));
    const physx::PxVec3 dir = (*target_pos - cur3);
    if (dir.magnitudeSquared() < 1e-6f) return;
    const physx::PxVec3 f = dir.getNormalized();
    const float yaw = std::atan2f(f.x, f.z);
    m_rotation = physx::PxQuat(yaw, PhysxApiWorldUp());
    syncAttachedActorsPose(logicalPose());
}

void SceneCharacterObj::factTo(physx::PxQuat* rotation)
{
    if (!rotation) return;
    m_rotation = *rotation;
    syncAttachedActorsPose(logicalPose());
}

bool SceneCharacterObj::addHitboxShape(physx::PxShape* shape, const physx::PxTransform& localPose)
{
    if (!m_controller) return false;
    return SceneObj::addHitboxShape(shape, localPose);
}

bool SceneCharacterObj::addTriggerShape(physx::PxShape* shape, const physx::PxTransform& localPose)
{
    if (!m_controller) return false;
    return SceneObj::addTriggerShape(shape, localPose);
}

