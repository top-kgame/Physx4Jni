//
// Created by zhangakai on 2026/4/13.
//

#ifndef PHYSXAPI_PHYSXCOMMON_H
#define PHYSXAPI_PHYSXCOMMON_H

#include "PxPhysicsAPI.h"
#include <cstdint>

enum class SCENE_OBJECT_TYPE
{
    STATIC, //静态物体
    RIGID,  //刚体
    KINEMATIC,  //运动学物体
    TRIGGER, //触发器
    CHARACTER,  //角色控制器
};

constexpr uint32_t PHYSXAPI_DEFAULT_LAYER = 1u;
constexpr uint32_t PHYSXAPI_ALL_LAYERS = 0xffffffffu;

enum PhysxApiObjectFlags : uint32_t
{
    PHYSXAPI_OBJECT_FLAG_NONE = 0,
    PHYSXAPI_OBJECT_FLAG_TRIGGER = 1u << 0,
    PHYSXAPI_OBJECT_FLAG_KINEMATIC = 1u << 1,
    PHYSXAPI_OBJECT_FLAG_CHARACTER = 1u << 2,
};

inline physx::PxFilterData MakePhysxApiSimulationFilterData(uint32_t layer,
                                                            uint32_t collideMask,
                                                            uint32_t objectFlags)
{
    return physx::PxFilterData(layer, collideMask, objectFlags, 0);
}

inline physx::PxFilterData MakePhysxApiQueryShapeFilterData(uint32_t layer, uint32_t objectFlags)
{
    return physx::PxFilterData(layer, 0, objectFlags, 0);
}

physx::PxFilterFlags PhysxApiSimulationFilterShader(physx::PxFilterObjectAttributes attributes0,
                                                    physx::PxFilterData filterData0,
                                                    physx::PxFilterObjectAttributes attributes1,
                                                    physx::PxFilterData filterData1,
                                                    physx::PxPairFlags& pairFlags,
                                                    const void* constantBlock,
                                                    physx::PxU32 constantBlockSize);
#endif //PHYSXAPI_PHYSXCOMMON_H