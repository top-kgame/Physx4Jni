#ifndef PHYSXAPI_LIBRARY_H
#define PHYSXAPI_LIBRARY_H
#include "PhysxApiCommon.h"
#include "scene/Scene.h"
#include "object/SceneObj.h"

void initPhysxApi();

Scene* createScene();
void updateScene(Scene* scene);
void destroyScene(Scene* scene);


#endif // PHYSXAPI_LIBRARY_H