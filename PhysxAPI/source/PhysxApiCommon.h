//
// Created by zhangakai on 2026/4/13.
//

#ifndef PHYSXAPI_PHYSXCOMMON_H
#define PHYSXAPI_PHYSXCOMMON_H

#include "PxPhysicsAPI.h"

enum class SCENE_OBJECT_TYPE
{
    STATIC, //静态物体
    RIGID,  //刚体
    KINEMATIC,  //运动学物体
    CHARACTER,  //角色控制器
};
#endif //PHYSXAPI_PHYSXCOMMON_H