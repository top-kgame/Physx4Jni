//
// Created by zhangakai on 2026/4/13.
//

#ifndef PHYSXAPI_SCENEOBJ_H
#define PHYSXAPI_SCENEOBJ_H

#include "../PhysxApiCommon.h"

#include <functional>
#include <vector>

class Scene;

class SceneObj
{
public:
    enum class ActorRole
    {
        UNKNOWN,
        PRIMARY,
        COLLIDER,
        HITBOX,
        HURTBOX,
        TRIGGER,
        CUSTOM,
    };

    struct ActorFilterConfig
    {
        uint32_t layer{PHYSXAPI_DEFAULT_LAYER};
        uint32_t collideMask{PHYSXAPI_ALL_LAYERS};
        uint32_t objectFlags{PHYSXAPI_OBJECT_FLAG_NONE};
    };

    struct ActorRecord
    {
        SceneObj* owner{nullptr};
        physx::PxRigidActor* actor{nullptr};
        ActorRole role{ActorRole::UNKNOWN};
        physx::PxTransform localPose{physx::PxIdentity};
        bool owned{true};
        bool syncPose{false};
        bool updateMassOnShapeChange{false};
        float density{PHYSXAPI_DEFAULT_DENSITY};
        ActorFilterConfig filter{};

        physx::PxShape* createBoxShape(const physx::PxVec3& halfExtents) const;
        physx::PxShape* createSphereShape(float radius) const;
        physx::PxShape* createCapsuleShape(float radius, float halfHeight) const;
        void setCollisionFilter(uint32_t layer, uint32_t collideMask);
        void setObjectFlags(uint32_t objectFlags);
        uint32_t layer() const { return filter.layer; }
        uint32_t collideMask() const { return filter.collideMask; }
        uint32_t objectFlags() const { return filter.objectFlags; }
        physx::PxFilterData simulationFilterData() const;
        physx::PxFilterData queryFilterData() const;
        void applyFilterDataToShape(physx::PxShape* shape) const;
        void refreshFilterData() const;
        bool attachShape(physx::PxShape* shape);
        bool detachShape(physx::PxShape* shape);
    };

    explicit SceneObj(Scene* owner);
    virtual ~SceneObj() = default;

    uint64_t handle() const { return m_handle; }

    ActorRecord* actorRecord(ActorRole role);
    const ActorRecord* actorRecord(ActorRole role) const;
    ActorRecord* primaryActorRecord() { return actorRecord(ActorRole::PRIMARY); }
    const ActorRecord* primaryActorRecord() const { return actorRecord(ActorRole::PRIMARY); }
    physx::PxRigidActor* primaryActor() const;
    bool ownsActor(const physx::PxActor* actor) const;
    void forEachActor(const std::function<void(const ActorRecord&)>& visitor) const;
    void forEachActor(ActorRole role, const std::function<void(const ActorRecord&)>& visitor) const;

    bool attachShape(ActorRole role, physx::PxShape* shape);
    bool detachShape(ActorRole role, physx::PxShape* shape);
    bool addHitboxShape(physx::PxShape* shape,
                        const physx::PxTransform& localPose = physx::PxTransform(physx::PxIdentity));
    bool addHurtboxShape(physx::PxShape* shape,
                         const physx::PxTransform& localPose = physx::PxTransform(physx::PxIdentity));
    bool addTriggerShape(physx::PxShape* shape,
                         const physx::PxTransform& localPose = physx::PxTransform(physx::PxIdentity));
    void syncAttachedActorsToLogicalPose();

    /** Scene::destroyObject 调用：从 scene 中移除底层实体并释放 */
    virtual void destroy() = 0;

    /**
     * 向某个方向移动
     * @param movement 朝向向量，包括方向和长度
     */
    virtual void move(const physx::PxVec3 *movement) = 0;

    /**
     * 朝某个坐标移动
     * @param target_pos 目标移动位置坐标
     */
    virtual void move_to(const physx::PxVec3 *target_pos) = 0;

    /**
     * 传送到某个位置
     * @param target_pos 传送位置坐标
     */
    virtual void teleport(const physx::PxVec3 *target_pos) = 0;

    /**
     * 朝向某个位置
     * @param target_pos 位置坐标
     */
    virtual void faceTo(const physx::PxVec3 *target_pos) = 0;

    /**
     * 按照旋转修改朝向
     * @param rotation 旋转
     */
    virtual void factTo(physx::PxQuat *rotation) = 0;

protected:
    ActorRecord* registerActor(physx::PxRigidActor* actor,
                               ActorRole role,
                               const physx::PxTransform& localPose,
                               bool owned,
                               bool syncPose,
                               const ActorFilterConfig& filter,
                               bool updateMassOnShapeChange = false,
                               float density = PHYSXAPI_DEFAULT_DENSITY);
    bool unregisterActor(physx::PxRigidActor* actor, bool releaseOwnedActor);
    void destroyRegisteredActors();
    ActorRecord* createAttachedKinematicActor(ActorRole role,
                                              const physx::PxTransform& localPose,
                                              const ActorFilterConfig& filter);
    bool addAttachedShape(ActorRole role,
                          physx::PxShape* shape,
                          const physx::PxTransform& localPose,
                          const ActorFilterConfig& filter);
    void syncAttachedActorsPose(const physx::PxTransform& logicalPose);
    virtual physx::PxTransform logicalPose() const = 0;

    Scene* m_owner_scene;
    uint64_t m_handle;
    std::vector<ActorRecord> m_actor_records;
};


#endif //PHYSXAPI_SCENEOBJ_H