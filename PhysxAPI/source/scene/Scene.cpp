//
// Created by zhangakai on 2026/2/12.
//

#include "Scene.h"

#include "../PhysxApi.h"

#include <PxPhysicsAPI.h>

#include "../object/SceneStaticObj.h"
#include "../object/SceneRigidObj.h"
#include "../object/SceneKinematicObj.h"
#include "../object/SceneTriggerObj.h"
#include "../object/SceneCharacterObj.h"

class SceneSimulationEventCallbackImpl final : public physx::PxSimulationEventCallback
{
public:
    explicit SceneSimulationEventCallbackImpl(Scene* owner) : m_owner(owner) {}

    void onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) override {}
    void onWake(physx::PxActor**, physx::PxU32) override {}
    void onSleep(physx::PxActor**, physx::PxU32) override {}
    void onAdvance(const physx::PxRigidBody* const*, const physx::PxTransform*, const physx::PxU32) override {}

    void onContact(const physx::PxContactPairHeader& pairHeader,
                   const physx::PxContactPair* pairs,
                   physx::PxU32 nbPairs) override;

    void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;

private:
    Scene* m_owner;
};

class SceneControllerHitReportImpl final : public physx::PxUserControllerHitReport
{
public:
    explicit SceneControllerHitReportImpl(Scene* owner) : m_owner(owner) {}

    void onShapeHit(const physx::PxControllerShapeHit& hit) override;
    void onControllerHit(const physx::PxControllersHit& hit) override;
    void onObstacleHit(const physx::PxControllerObstacleHit& hit) override;

private:
    Scene* m_owner;
};

namespace
{
physx::PxQueryFilterData MakeQueryMaskFilterData(uint32_t queryMask)
{
    physx::PxQueryFilterData filterData;
    filterData.data = physx::PxFilterData(queryMask, 0, 0, 0);
    filterData.flags = physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER;
    return filterData;
}

SceneObj* GetSceneObjFromActor(const physx::PxActor* actor)
{
    return actor ? static_cast<SceneObj*>(actor->userData) : nullptr;
}

SceneObj* GetSceneObjFromController(const physx::PxController* controller)
{
    return controller ? static_cast<SceneObj*>(controller->getUserData()) : nullptr;
}

physx::PxVec3 ToPxVec3(const physx::PxExtendedVec3& value)
{
    return physx::PxVec3(static_cast<float>(value.x),
                         static_cast<float>(value.y),
                         static_cast<float>(value.z));
}

class QueryMaskFilterCallback final : public physx::PxQueryFilterCallback
{
public:
    physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& queryFilterData,
                                          const physx::PxShape* shape,
                                          const physx::PxRigidActor*,
                                          physx::PxHitFlags&) override
    {
        if (!shape) return physx::PxQueryHitType::eNONE;
        const physx::PxFilterData shapeFilterData = shape->getQueryFilterData();
        return (shapeFilterData.word0 & queryFilterData.word0) != 0
                   ? physx::PxQueryHitType::eBLOCK
                   : physx::PxQueryHitType::eNONE;
    }

    physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData&,
                                           const physx::PxQueryHit&,
                                           const physx::PxShape*,
                                           const physx::PxRigidActor*) override
    {
        return physx::PxQueryHitType::eBLOCK;
    }
};

class IgnoreActorQueryMaskFilterCallback final : public physx::PxQueryFilterCallback
{
public:
    explicit IgnoreActorQueryMaskFilterCallback(const physx::PxRigidActor* ignoreActor)
        : m_ignore_actor(ignoreActor)
    {
    }

    physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& queryFilterData,
                                          const physx::PxShape* shape,
                                          const physx::PxRigidActor* actor,
                                          physx::PxHitFlags&) override
    {
        if (actor && m_ignore_actor && actor == m_ignore_actor) return physx::PxQueryHitType::eNONE;
        if (!shape) return physx::PxQueryHitType::eNONE;
        const physx::PxFilterData shapeFilterData = shape->getQueryFilterData();
        return (shapeFilterData.word0 & queryFilterData.word0) != 0
                   ? physx::PxQueryHitType::eBLOCK
                   : physx::PxQueryHitType::eNONE;
    }

    physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData&,
                                           const physx::PxQueryHit&,
                                           const physx::PxShape*,
                                           const physx::PxRigidActor*) override
    {
        return physx::PxQueryHitType::eBLOCK;
    }

private:
    const physx::PxRigidActor* m_ignore_actor;
};
} // namespace

void SceneSimulationEventCallbackImpl::onContact(const physx::PxContactPairHeader& pairHeader,
                                                 const physx::PxContactPair* pairs,
                                                 physx::PxU32 nbPairs)
{
    if (!m_owner) return;

    const SceneObj* objA = GetSceneObjFromActor(pairHeader.actors[0]);
    const SceneObj* objB = GetSceneObjFromActor(pairHeader.actors[1]);
    if (!objA || !objB) return;

    for (physx::PxU32 i = 0; i < nbPairs; ++i)
    {
        const physx::PxContactPair& pair = pairs[i];
        physx::PxContactPairPoint point{};
        const physx::PxU32 pointCount = pair.extractContacts(&point, 1);
        const physx::PxVec3 contactPoint = pointCount > 0 ? point.position : physx::PxVec3(0.0f);

        if (pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            m_owner->pushContactEvent(SceneContactEvent::Type::FOUND, objA->handle(), objB->handle(), contactPoint);
        }
        if (pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
        {
            m_owner->pushContactEvent(SceneContactEvent::Type::PERSIST, objA->handle(), objB->handle(), contactPoint);
        }
        if (pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            m_owner->pushContactEvent(SceneContactEvent::Type::LOST, objA->handle(), objB->handle(), physx::PxVec3(0.0f));
        }
    }
}

void SceneSimulationEventCallbackImpl::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
    if (!m_owner) return;

    for (physx::PxU32 i = 0; i < count; ++i)
    {
        const physx::PxTriggerPair& pair = pairs[i];
        if ((pair.flags & physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER) ||
            (pair.flags & physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
        {
            continue;
        }

        const SceneObj* triggerObj = GetSceneObjFromActor(pair.triggerActor);
        const SceneObj* otherObj = GetSceneObjFromActor(pair.otherActor);
        if (!triggerObj || !otherObj) continue;

        const Scene::TriggerPairKey key(triggerObj->handle(), otherObj->handle());
        if (pair.status & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            m_owner->m_current_trigger_pairs.insert(key);
            m_owner->m_found_trigger_pairs.insert(key);
            m_owner->pushTriggerEvent(SceneTriggerEvent::Type::ENTER, key.first, key.second);
        }
        if (pair.status & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            m_owner->m_current_trigger_pairs.erase(key);
            m_owner->pushTriggerEvent(SceneTriggerEvent::Type::EXIT, key.first, key.second);
        }
    }
}

void SceneControllerHitReportImpl::onShapeHit(const physx::PxControllerShapeHit& hit)
{
    if (!m_owner) return;
    const SceneObj* controllerObj = GetSceneObjFromController(hit.controller);
    const SceneObj* otherObj = GetSceneObjFromActor(hit.actor);
    if (!controllerObj) return;
    m_owner->pushCharacterHitEvent(SceneCharacterHitEvent::Type::SHAPE_HIT,
                                   controllerObj->handle(),
                                   otherObj ? otherObj->handle() : 0,
                                   ToPxVec3(hit.worldPos),
                                   hit.worldNormal);
}

void SceneControllerHitReportImpl::onControllerHit(const physx::PxControllersHit& hit)
{
    if (!m_owner) return;
    const SceneObj* controllerObj = GetSceneObjFromController(hit.controller);
    const SceneObj* otherObj = GetSceneObjFromController(hit.other);
    if (!controllerObj) return;
    m_owner->pushCharacterHitEvent(SceneCharacterHitEvent::Type::CONTROLLER_HIT,
                                   controllerObj->handle(),
                                   otherObj ? otherObj->handle() : 0,
                                   ToPxVec3(hit.worldPos),
                                   hit.worldNormal);
}

void SceneControllerHitReportImpl::onObstacleHit(const physx::PxControllerObstacleHit& hit)
{
    if (!m_owner) return;
    const SceneObj* controllerObj = GetSceneObjFromController(hit.controller);
    if (!controllerObj) return;
    m_owner->pushCharacterHitEvent(SceneCharacterHitEvent::Type::OBSTACLE_HIT,
                                   controllerObj->handle(),
                                   0,
                                   ToPxVec3(hit.worldPos),
                                   hit.worldNormal);
}

Scene::Scene(float fixed_dt)
    : m_px_scene(nullptr),
      m_controller_manager(nullptr),
      m_simulation_event_callback(nullptr),
      m_controller_hit_report(nullptr),
      m_fixed_dt(fixed_dt),
      m_character_up(0.0f, 1.0f, 0.0f),
      m_character_slope_limit(0.0f),
      m_character_step_offset(0.0f),
      m_character_contact_offset(0.1f),
      m_character_min_move_dist(0.001f)
{
}

Scene::~Scene()
{
    shutdown();
}

bool Scene::initialize()
{
    auto* physics = getPxPhysics();
    if (!physics) return false;

    if (!m_simulation_event_callback) m_simulation_event_callback = new SceneSimulationEventCallbackImpl(this);
    if (!m_controller_hit_report) m_controller_hit_report = new SceneControllerHitReportImpl(this);

    physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = getCpuDispatcher();
    sceneDesc.filterShader = PhysxApiSimulationFilterShader;
    sceneDesc.simulationEventCallback = m_simulation_event_callback;

    m_px_scene = physics->createScene(sceneDesc);
    if (!m_px_scene) return false;

    m_controller_manager = PxCreateControllerManager(*m_px_scene);
    return m_controller_manager != nullptr;
}

void Scene::shutdown()
{
    if (m_controller_manager)
    {
        m_controller_manager->release();
        m_controller_manager = nullptr;
    }
    if (m_px_scene)
    {
        m_px_scene->release();
        m_px_scene = nullptr;
    }
    delete m_simulation_event_callback;
    m_simulation_event_callback = nullptr;
    delete m_controller_hit_report;
    m_controller_hit_report = nullptr;
    m_contact_events.clear();
    m_trigger_events.clear();
    m_character_hit_events.clear();
    m_pending_character_hit_events.clear();
    m_previous_trigger_pairs.clear();
    m_current_trigger_pairs.clear();
    m_found_trigger_pairs.clear();
}

void Scene::update()
{
    if (!m_px_scene) return;
    beginUpdateEventCollection();
    const float dt = (m_fixed_dt > 0.0f) ? m_fixed_dt : (1.0f / 60.0f);
    m_px_scene->simulate(dt);
    m_px_scene->fetchResults(true);
    finalizeTriggerEvents();
}

SceneObj* Scene::createStaticObject()
{
    auto* obj = new SceneStaticObj(this);
    if (!obj->initialize())
    {
        delete obj;
        return nullptr;
    }
    return obj;
}

SceneObj* Scene::createRigidObject()
{
    auto* obj = new SceneRigidObj(this);
    if (!obj->initialize(false))
    {
        delete obj;
        return nullptr;
    }
    return obj;
}

SceneObj* Scene::createKinematicObject()
{
    auto* obj = new SceneKinematicObj(this);
    if (!obj->initialize())
    {
        delete obj;
        return nullptr;
    }
    return obj;
}

SceneObj* Scene::createTriggerObject()
{
    auto* obj = new SceneTriggerObj(this);
    if (!obj->initialize())
    {
        delete obj;
        return nullptr;
    }
    return obj;
}

SceneObj* Scene::createCharacterObject(float radius, float height, const physx::PxExtendedVec3& position)
{
    auto* obj = new SceneCharacterObj(this, radius, height, position);
    if (!obj->initialize())
    {
        delete obj;
        return nullptr;
    }
    return obj;
}

void Scene::destroyObject(SceneObj* obj)
{
    if (!obj) return;
    obj->destroy();
    delete obj;
}

QueryResult Scene::rayCast(const physx::PxVec3& origin,
                           const physx::PxVec3& unitDir,
                           float distance,
                           uint32_t queryMask)
{
    if (!m_px_scene || distance <= 0.0f || unitDir.magnitudeSquared() < 1e-6f) return {};

    physx::PxRaycastBuffer hit;
    const physx::PxQueryFilterData filterData = MakeQueryMaskFilterData(queryMask);

    QueryMaskFilterCallback filterCallback;
    const bool hasHit = m_px_scene->raycast(origin,
                                            unitDir.getNormalized(),
                                            distance,
                                            hit,
                                            physx::PxHitFlag::eDEFAULT,
                                            filterData,
                                            &filterCallback);
    if (!hasHit || !hit.hasBlock) return {};
    return QueryResult::fromRaycastHit(hit.block);
}

QueryBatchResult Scene::rayCastBatch(const std::vector<RayCastRequest>& requests)
{
    QueryBatchResult result;
    for (const RayCastRequest& request : requests)
    {
        result.add(rayCast(request.origin, request.unitDir, request.distance, request.queryMask));
    }
    return result;
}

QueryResult Scene::sweep(const physx::PxGeometry& geometry,
                         const physx::PxTransform& pose,
                         const physx::PxVec3& unitDir,
                         float distance,
                         uint32_t queryMask)
{
    if (!m_px_scene || distance <= 0.0f || unitDir.magnitudeSquared() < 1e-6f) return {};

    physx::PxSweepBuffer hit;
    const physx::PxQueryFilterData filterData = MakeQueryMaskFilterData(queryMask);

    QueryMaskFilterCallback filterCallback;
    const bool hasHit = m_px_scene->sweep(geometry,
                                          pose,
                                          unitDir.getNormalized(),
                                          distance,
                                          hit,
                                          physx::PxHitFlag::eDEFAULT,
                                          filterData,
                                          &filterCallback);
    if (!hasHit || !hit.hasBlock) return {};
    return QueryResult::fromSweepHit(hit.block);
}

QueryResult Scene::sweep(const physx::PxGeometry& geometry,
                         const physx::PxTransform& pose,
                         const physx::PxVec3& unitDir,
                         float distance,
                         uint32_t queryMask,
                         const physx::PxRigidActor* ignoreActor)
{
    if (!m_px_scene || distance <= 0.0f || unitDir.magnitudeSquared() < 1e-6f) return {};

    physx::PxSweepBuffer hit;
    const physx::PxQueryFilterData filterData = MakeQueryMaskFilterData(queryMask);

    IgnoreActorQueryMaskFilterCallback filterCallback(ignoreActor);
    const bool hasHit = m_px_scene->sweep(geometry,
                                          pose,
                                          unitDir.getNormalized(),
                                          distance,
                                          hit,
                                          physx::PxHitFlag::eDEFAULT,
                                          filterData,
                                          &filterCallback);
    if (!hasHit || !hit.hasBlock) return {};
    return QueryResult::fromSweepHit(hit.block);
}

QueryBatchResult Scene::sweepBatch(const std::vector<SweepRequest>& requests)
{
    QueryBatchResult result;
    for (const SweepRequest& request : requests)
    {
        result.add(sweep(request.geometry.any(),
                         request.pose,
                         request.unitDir,
                         request.distance,
                         request.queryMask));
    }
    return result;
}

QueryResult Scene::overlap(const physx::PxGeometry& geometry,
                           const physx::PxTransform& pose,
                           uint32_t queryMask)
{
    if (!m_px_scene) return {};

    physx::PxOverlapBuffer hit;
    physx::PxQueryFilterData filterData = MakeQueryMaskFilterData(queryMask);
    filterData.flags |= physx::PxQueryFlag::eANY_HIT;

    QueryMaskFilterCallback filterCallback;
    const bool hasHit = m_px_scene->overlap(geometry,
                                            pose,
                                            hit,
                                            filterData,
                                            &filterCallback);
    if (!hasHit || !hit.hasBlock) return {};
    return QueryResult::fromOverlapHit(hit.block);
}

QueryBatchResult Scene::overlapBatch(const std::vector<OverlapRequest>& requests)
{
    QueryBatchResult result;
    for (const OverlapRequest& request : requests)
    {
        result.add(overlap(request.geometry.any(), request.pose, request.queryMask));
    }
    return result;
}

physx::PxUserControllerHitReport* Scene::controllerHitReport() const
{
    return m_controller_hit_report;
}

void Scene::beginUpdateEventCollection()
{
    m_contact_events.clear();
    m_trigger_events.clear();
    m_character_hit_events.clear();
    m_current_trigger_pairs = m_previous_trigger_pairs;
    m_found_trigger_pairs.clear();
}

void Scene::finalizeTriggerEvents()
{
    for (const TriggerPairKey& pair : m_current_trigger_pairs)
    {
        if (m_previous_trigger_pairs.find(pair) != m_previous_trigger_pairs.end() &&
            m_found_trigger_pairs.find(pair) == m_found_trigger_pairs.end())
        {
            pushTriggerEvent(SceneTriggerEvent::Type::PERSIST, pair.first, pair.second);
        }
    }
    m_previous_trigger_pairs = m_current_trigger_pairs;
    m_character_hit_events.swap(m_pending_character_hit_events);
    m_pending_character_hit_events.clear();
}

void Scene::pushContactEvent(SceneContactEvent::Type type,
                             uint64_t objectAHandle,
                             uint64_t objectBHandle,
                             const physx::PxVec3& point)
{
    m_contact_events.push_back(SceneContactEvent{objectAHandle, objectBHandle, point, type});
}

void Scene::pushTriggerEvent(SceneTriggerEvent::Type type, uint64_t triggerHandle, uint64_t otherHandle)
{
    m_trigger_events.push_back(SceneTriggerEvent{triggerHandle, otherHandle, type});
}

void Scene::pushCharacterHitEvent(SceneCharacterHitEvent::Type type,
                                  uint64_t controllerHandle,
                                  uint64_t otherHandle,
                                  const physx::PxVec3& worldPos,
                                  const physx::PxVec3& worldNormal)
{
    m_pending_character_hit_events.push_back(SceneCharacterHitEvent{controllerHandle, otherHandle, worldPos, worldNormal, type});
}

