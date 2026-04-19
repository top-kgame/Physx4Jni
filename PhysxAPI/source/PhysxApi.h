#ifndef PHYSXAPI_LIBRARY_H
#define PHYSXAPI_LIBRARY_H
#include "PhysxApiCommon.h"
#include "scene/Scene.h"
#include "object/SceneObj.h"

void initPhysxApi();
void shutdownPhysxApi();

// fixed_dt: 固定时间步长（创建 scene 时传入）
Scene* createScene(float fixed_dt);
void updateScene(Scene* scene);
void destroyScene(Scene* scene);

// 仅供 C++ 封装层内部使用的全局访问器（JNI 层暂不关心）
physx::PxPhysics* getPxPhysics();
physx::PxMaterial* getDefaultMaterial();
physx::PxCpuDispatcher* getCpuDispatcher();


#endif // PHYSXAPI_LIBRARY_H