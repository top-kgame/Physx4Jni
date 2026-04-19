//
// Created by zhangakai on 2026/4/13.
//

#include "SceneObj.h"

#include "../PhysxApi.h"
#include "../scene/Scene.h"

SceneObj::SceneObj(Scene* owner)
    : m_owner_scene(owner),
      m_handle(reinterpret_cast<uint64_t>(this)),
      m_layer(PHYSXAPI_DEFAULT_LAYER),
      m_collide_mask(PHYSXAPI_ALL_LAYERS),
      m_object_flags(PHYSXAPI_OBJECT_FLAG_NONE)
{
}

physx::PxShape* SceneObj::createBoxShape(const physx::PxVec3& halfExtents)
{
    auto* physics = getPxPhysics();
    auto* mat = getDefaultMaterial();
    if (!physics || !mat) return nullptr;
    return physics->createShape(physx::PxBoxGeometry(halfExtents), *mat, true);
}

physx::PxShape* SceneObj::createSphereShape(float radius)
{
    auto* physics = getPxPhysics();
    auto* mat = getDefaultMaterial();
    if (!physics || !mat) return nullptr;
    return physics->createShape(physx::PxSphereGeometry(radius), *mat, true);
}

physx::PxShape* SceneObj::createCapsuleShape(float radius, float halfHeight)
{
    auto* physics = getPxPhysics();
    auto* mat = getDefaultMaterial();
    if (!physics || !mat) return nullptr;
    return physics->createShape(physx::PxCapsuleGeometry(radius, halfHeight), *mat, true);
}

void SceneObj::setCollisionFilter(uint32_t layer, uint32_t collideMask)
{
    m_layer = layer;
    m_collide_mask = collideMask;
    refreshFilterData();
}

physx::PxFilterData SceneObj::simulationFilterData() const
{
    return MakePhysxApiSimulationFilterData(m_layer, m_collide_mask, m_object_flags);
}

physx::PxFilterData SceneObj::queryFilterData() const
{
    return MakePhysxApiQueryShapeFilterData(m_layer, m_object_flags);
}

void SceneObj::applyFilterDataToShape(physx::PxShape* shape) const
{
    if (!shape) return;
    shape->setSimulationFilterData(simulationFilterData());
    shape->setQueryFilterData(queryFilterData());
}

void SceneObj::setObjectFlags(uint32_t objectFlags)
{
    m_object_flags = objectFlags;
    refreshFilterData();
}
