//
// Created by zhangakai on 2026/4/13.
//

#include "QueryResult.h"

#include "../object/SceneObj.h"

namespace
{
uint64_t GetHandleFromActor(const physx::PxRigidActor* actor)
{
    const auto* obj = actor ? static_cast<const SceneObj*>(actor->userData) : nullptr;
    return obj ? obj->handle() : 0;
}
} // namespace

QueryResult::QueryResult()
    : hasHit(false),
      objectHandle(0),
      distance(0.0f),
      position(0.0f),
      normal(0.0f),
      actor(nullptr),
      shape(nullptr)
{
}

QueryResult QueryResult::fromRaycastHit(const physx::PxRaycastHit& hit)
{
    QueryResult result;
    result.hasHit = true;
    result.objectHandle = GetHandleFromActor(hit.actor);
    result.distance = hit.distance;
    result.position = hit.position;
    result.normal = hit.normal;
    result.actor = hit.actor;
    result.shape = hit.shape;
    return result;
}

QueryResult QueryResult::fromSweepHit(const physx::PxSweepHit& hit)
{
    QueryResult result;
    result.hasHit = true;
    result.objectHandle = GetHandleFromActor(hit.actor);
    result.distance = hit.distance;
    result.position = hit.position;
    result.normal = hit.normal;
    result.actor = hit.actor;
    result.shape = hit.shape;
    return result;
}

QueryResult QueryResult::fromOverlapHit(const physx::PxOverlapHit& hit)
{
    QueryResult result;
    result.hasHit = true;
    result.objectHandle = GetHandleFromActor(hit.actor);
    result.distance = 0.0f;
    result.position = physx::PxVec3(0.0f);
    result.normal = physx::PxVec3(0.0f);
    result.actor = hit.actor;
    result.shape = hit.shape;
    return result;
}

std::size_t QueryBatchResult::hitCount() const
{
    std::size_t count = 0;
    for (const QueryResult& entry : m_entries)
    {
        if (entry.hasHit) ++count;
    }
    return count;
}