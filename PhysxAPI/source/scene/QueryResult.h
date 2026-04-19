//
// Created by zhangakai on 2026/4/13.
//

#ifndef PHYSXAPI_QUERYRESULT_H
#define PHYSXAPI_QUERYRESULT_H

#include <cstddef>
#include <vector>

#include "../PhysxApiCommon.h"

struct RayCastRequest
{
    physx::PxVec3 origin;
    physx::PxVec3 unitDir;
    float distance;
    uint32_t queryMask;

    RayCastRequest(const physx::PxVec3& inOrigin,
                   const physx::PxVec3& inUnitDir,
                   float inDistance,
                   uint32_t inQueryMask = PHYSXAPI_ALL_LAYERS)
        : origin(inOrigin), unitDir(inUnitDir), distance(inDistance), queryMask(inQueryMask)
    {
    }
};

struct SweepRequest
{
    physx::PxGeometryHolder geometry;
    physx::PxTransform pose;
    physx::PxVec3 unitDir;
    float distance;
    uint32_t queryMask;

    SweepRequest(const physx::PxGeometry& inGeometry,
                 const physx::PxTransform& inPose,
                 const physx::PxVec3& inUnitDir,
                 float inDistance,
                 uint32_t inQueryMask = PHYSXAPI_ALL_LAYERS)
        : geometry(inGeometry), pose(inPose), unitDir(inUnitDir), distance(inDistance), queryMask(inQueryMask)
    {
    }
};

struct OverlapRequest
{
    physx::PxGeometryHolder geometry;
    physx::PxTransform pose;
    uint32_t queryMask;

    OverlapRequest(const physx::PxGeometry& inGeometry,
                   const physx::PxTransform& inPose,
                   uint32_t inQueryMask = PHYSXAPI_ALL_LAYERS)
        : geometry(inGeometry), pose(inPose), queryMask(inQueryMask)
    {
    }
};

class QueryResult
{
public:
    bool hasHit;
    uint64_t objectHandle;
    float distance;
    physx::PxVec3 position;
    physx::PxVec3 normal;
    const physx::PxRigidActor* actor;
    const physx::PxShape* shape;

    QueryResult();
    static QueryResult fromRaycastHit(const physx::PxRaycastHit& hit);
    static QueryResult fromSweepHit(const physx::PxSweepHit& hit);
    static QueryResult fromOverlapHit(const physx::PxOverlapHit& hit);
};

class QueryBatchResult
{
public:
    const std::vector<QueryResult>& entries() const { return m_entries; }
    std::size_t size() const { return m_entries.size(); }
    std::size_t hitCount() const;
    const QueryResult& operator[](std::size_t index) const { return m_entries[index]; }
    void add(const QueryResult& result) { m_entries.push_back(result); }

private:
    std::vector<QueryResult> m_entries;
};

#endif //PHYSXAPI_QUERYRESULT_H