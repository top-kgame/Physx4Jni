//
// Created by zhangakai on 2026/4/13.
//

#ifndef PHYSXAPI_SCENEOBJ_H
#define PHYSXAPI_SCENEOBJ_H

#include "../PhysxApiCommon.h"

class SceneObj
{
public:
    SceneObj();
    physx::PxShape* createBoxShape();
    physx::PxShape* createSphereShape();
    physx::PxShape* createCapsuleShape();
    void detachShape(physx::PxShape* shape);

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

    virtual ~SceneObj() = default;
};


#endif //PHYSXAPI_SCENEOBJ_H