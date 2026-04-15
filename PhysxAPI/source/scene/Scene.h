//
// Created by zhangakai on 2026/2/12.
//

#ifndef PHYSXAPI_SCENE_H
#define PHYSXAPI_SCENE_H

#include "QueryResult.h"
#include "../PhysxApiCommon.h"
#include "object/SceneObj.h"

class Scene
{
public:
    Scene();
    ~Scene();
    bool initialize();
    void shutdown();
    void update();

    SceneObj* createObject(SCENE_OBJECT_TYPE type);
    void destroyObject(SceneObj* obj);

    QueryResult rayCast();
    QueryResult sweep();
    QueryResult overlap();
private:
    physx::PxScene * m_px_scene;
};


#endif //PHYSXAPI_SCENE_H