//
// Created by zhangakai on 2026/4/13.
//

#ifndef PHYSXAPI_SCENEOBJ_H
#define PHYSXAPI_SCENEOBJ_H

#include "../PhysxApiCommon.h"

class Scene;

class SceneObj
{
public:
    explicit SceneObj(Scene* owner);
    virtual ~SceneObj() = default;

    physx::PxShape* createBoxShape(const physx::PxVec3& halfExtents);
    physx::PxShape* createSphereShape(float radius);
    physx::PxShape* createCapsuleShape(float radius, float halfHeight);

    void setCollisionFilter(uint32_t layer, uint32_t collideMask);
    uint64_t handle() const { return m_handle; }
    uint32_t layer() const { return m_layer; }
    uint32_t collideMask() const { return m_collide_mask; }
    uint32_t objectFlags() const { return m_object_flags; }
    /**
     * detach 并释放 shape（如果仍挂在 actor 上，会先 detach 再 release）
     * @return 是否成功执行（shape 为 null 返回 false）
     */
    virtual bool detachShape(physx::PxShape* shape) = 0;

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
    [[nodiscard]] physx::PxFilterData simulationFilterData() const;
    physx::PxFilterData queryFilterData() const;
    void applyFilterDataToShape(physx::PxShape* shape) const;
    void setObjectFlags(uint32_t objectFlags);
    virtual void refreshFilterData() = 0;

    Scene* m_owner_scene;
    uint64_t m_handle;
    uint32_t m_layer;
    uint32_t m_collide_mask;
    uint32_t m_object_flags;
};


#endif //PHYSXAPI_SCENEOBJ_H