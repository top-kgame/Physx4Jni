//
// Created by zhangakai on 2026/2/12.
//

#ifndef PHYSXAPI_SCENE_H
#define PHYSXAPI_SCENE_H

#include <set>
#include <utility>
#include <vector>

#include "QueryResult.h"
#include "object/SceneObj.h"

class SceneSimulationEventCallbackImpl;
class SceneControllerHitReportImpl;

struct SceneContactEvent
{
    enum class Type
    {
        FOUND,
        PERSIST,
        LOST,
    };

    uint64_t objectAHandle;
    uint64_t objectBHandle;
    physx::PxVec3 point;
    Type type;
};

struct SceneTriggerEvent
{
    enum class Type
    {
        ENTER,
        PERSIST,
        EXIT,
    };

    uint64_t triggerHandle;
    uint64_t otherHandle;
    Type type;
};

struct SceneCharacterHitEvent
{
    enum class Type
    {
        SHAPE_HIT,
        CONTROLLER_HIT,
        OBSTACLE_HIT,
    };

    uint64_t controllerHandle;
    uint64_t otherHandle;
    physx::PxVec3 worldPos;
    physx::PxVec3 worldNormal;
    Type type;
};

class Scene
{
public:
    explicit Scene(float fixed_dt);
    ~Scene();
    bool initialize();
    void shutdown();
    void update();

    SceneObj* createStaticObject();
    SceneObj* createRigidObject();
    SceneObj* createKinematicObject();
    SceneObj* createTriggerObject();
    SceneObj* createCharacterObject(float radius, float height, const physx::PxExtendedVec3& position);
    void destroyObject(SceneObj* obj);

    QueryResult rayCast(const physx::PxVec3& origin,
                        const physx::PxVec3& unitDir,
                        float distance,
                        uint32_t queryMask = PHYSXAPI_ALL_LAYERS);
    QueryBatchResult rayCastBatch(const std::vector<RayCastRequest>& requests);
    QueryResult sweep(const physx::PxGeometry& geometry,
                      const physx::PxTransform& pose,
                      const physx::PxVec3& unitDir,
                      float distance,
                      uint32_t queryMask = PHYSXAPI_ALL_LAYERS);
    QueryBatchResult sweepBatch(const std::vector<SweepRequest>& requests);
    QueryResult overlap(const physx::PxGeometry& geometry,
                        const physx::PxTransform& pose,
                        uint32_t queryMask = PHYSXAPI_ALL_LAYERS);
    QueryBatchResult overlapBatch(const std::vector<OverlapRequest>& requests);

    const std::vector<SceneContactEvent>& contactEvents() const { return m_contact_events; }
    const std::vector<SceneTriggerEvent>& triggerEvents() const { return m_trigger_events; }
    const std::vector<SceneCharacterHitEvent>& characterHitEvents() const { return m_character_hit_events; }
    
    physx::PxScene* pxScene() const { return m_px_scene; }
    physx::PxControllerManager* controllerManager() const { return m_controller_manager; }
    physx::PxUserControllerHitReport* controllerHitReport() const;

    /** 固定时间步（用于 simulate / CCT move 的 elapsedTime），<=0 时回落到 1/60 */
    float fixedDeltaTime() const { return (m_fixed_dt > 0.0f) ? m_fixed_dt : (1.0f / 60.0f); }

    /** 角色控制器共用：世界向上轴（建议单位向量） */
    physx::PxVec3 characterControllerUpDirection() const { return m_character_up; }
    float characterControllerSlopeLimit() const { return m_character_slope_limit; }
    float characterControllerStepOffset() const { return m_character_step_offset; }
    float characterControllerContactOffset() const { return m_character_contact_offset; }
    float characterControllerMinMoveDistance() const { return m_character_min_move_dist; }

private:
    friend class SceneSimulationEventCallbackImpl;
    friend class SceneControllerHitReportImpl;

    using TriggerPairKey = std::pair<uint64_t, uint64_t>;

    void beginUpdateEventCollection();
    void finalizeTriggerEvents();
    void pushContactEvent(SceneContactEvent::Type type,
                          uint64_t objectAHandle,
                          uint64_t objectBHandle,
                          const physx::PxVec3& point);
    void pushTriggerEvent(SceneTriggerEvent::Type type, uint64_t triggerHandle, uint64_t otherHandle);
    void pushCharacterHitEvent(SceneCharacterHitEvent::Type type,
                               uint64_t controllerHandle,
                               uint64_t otherHandle,
                               const physx::PxVec3& worldPos,
                               const physx::PxVec3& worldNormal);

    physx::PxScene * m_px_scene;
    physx::PxControllerManager* m_controller_manager;
    SceneSimulationEventCallbackImpl* m_simulation_event_callback;
    SceneControllerHitReportImpl* m_controller_hit_report;
    float m_fixed_dt;

    physx::PxVec3 m_character_up;
    float m_character_slope_limit;
    float m_character_step_offset;
    float m_character_contact_offset;
    float m_character_min_move_dist;

    std::vector<SceneContactEvent> m_contact_events;
    std::vector<SceneTriggerEvent> m_trigger_events;
    std::vector<SceneCharacterHitEvent> m_character_hit_events;
    std::vector<SceneCharacterHitEvent> m_pending_character_hit_events;
    std::set<TriggerPairKey> m_previous_trigger_pairs;
    std::set<TriggerPairKey> m_current_trigger_pairs;
    std::set<TriggerPairKey> m_found_trigger_pairs;
};


#endif //PHYSXAPI_SCENE_H