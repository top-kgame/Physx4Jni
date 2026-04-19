#include "PhysxApi.h"

#include <mutex>
#include <set>
#include <thread>

#include "PhysxApiAllocator.h"

static physx::PxFoundation* gFoundation = nullptr;
static physx::PxDefaultCpuDispatcher* gDispatcher = nullptr;
static physx::PxPhysics *gPhysics = nullptr;
static physx::PxMaterial *gMaterial = nullptr;
static physx::PxPvd *gPvd = nullptr;

static PhysxApiAllocator gAllocator;
static physx::PxDefaultErrorCallback gErrorCallback;

static std::set<Scene *> gPhysXSceneMap;
static std::mutex gCreateSceneMtx;
static std::mutex gDestroySceneMtx;

physx::PxFilterFlags PhysxApiSimulationFilterShader(physx::PxFilterObjectAttributes attributes0,
                                                    physx::PxFilterData filterData0,
                                                    physx::PxFilterObjectAttributes attributes1,
                                                    physx::PxFilterData filterData1,
                                                    physx::PxPairFlags& pairFlags,
                                                    const void*,
                                                    physx::PxU32)
{
    const bool allowAtoB = (filterData0.word0 & filterData1.word1) != 0;
    const bool allowBtoA = (filterData1.word0 & filterData0.word1) != 0;
    if (!allowAtoB || !allowBtoA)
    {
        return physx::PxFilterFlag::eSUPPRESS;
    }

    const bool isTrigger = physx::PxFilterObjectIsTrigger(attributes0) ||
                           physx::PxFilterObjectIsTrigger(attributes1) ||
                           ((filterData0.word2 | filterData1.word2) & PHYSXAPI_OBJECT_FLAG_TRIGGER) != 0;
    if (isTrigger)
    {
        pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
        return physx::PxFilterFlag::eDEFAULT;
    }

    pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT |
                physx::PxPairFlag::eNOTIFY_TOUCH_FOUND |
                physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
                physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
    return physx::PxFilterFlag::eDEFAULT;
}

void initPhysxApi()
{
    if (gFoundation != nullptr) return;

    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    if (!gFoundation)
    {
        gErrorCallback.reportError(physx::PxErrorCode::eABORT, "PxCreateFoundation failed!", __FILE__, __LINE__);
        return;
    }

    const uint32_t threads = std::max(1u, std::thread::hardware_concurrency());
    // 物理模拟一般不需要占满所有核，取一个保守值即可
    gDispatcher = physx::PxDefaultCpuDispatcherCreate(std::min(threads, 4u));

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, physx::PxTolerancesScale(), true, gPvd);
    if (!gPhysics)
    {
        gErrorCallback.reportError(physx::PxErrorCode::eABORT, "PxCreatePhysics failed!", __FILE__, __LINE__);
        return;
    }
    if (!PxInitExtensions(*gPhysics, gPvd))
    {
        gErrorCallback.reportError(physx::PxErrorCode::eABORT, "PxInitExtensions failed!", __FILE__, __LINE__);
    }
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0);
}

void shutdownPhysxApi()
{
    // 先确保所有 scene 都被销毁，避免 physics/foundation 先释放造成崩溃
    {
        std::scoped_lock lock(gCreateSceneMtx, gDestroySceneMtx);
        for (Scene* s : gPhysXSceneMap)
        {
            if (!s) continue;
            s->shutdown();
            delete s;
        }
        gPhysXSceneMap.clear();
    }

    if (gMaterial)
    {
        gMaterial->release();
        gMaterial = nullptr;
    }

    if (gPhysics)
    {
        PxCloseExtensions();
        gPhysics->release();
        gPhysics = nullptr;
    }

    if (gDispatcher)
    {
        gDispatcher->release();
        gDispatcher = nullptr;
    }

    if (gPvd)
    {
        gPvd->release();
        gPvd = nullptr;
    }

    if (gFoundation)
    {
        gFoundation->release();
        gFoundation = nullptr;
    }
}

physx::PxPhysics* getPxPhysics()
{
    return gPhysics;
}

physx::PxMaterial* getDefaultMaterial()
{
    return gMaterial;
}

physx::PxCpuDispatcher* getCpuDispatcher()
{
    return gDispatcher;
}

Scene* createScene(float fixed_dt)
{
    std::lock_guard<std::mutex> lock(gCreateSceneMtx);
    auto* scene = new Scene(fixed_dt);
    if (!scene->initialize())
    {
        delete scene;
        return nullptr;
    }
    gPhysXSceneMap.insert(scene);
    return scene;
}

void updateScene(Scene* scene)
{
    if (!scene) return;
    scene->update();
}

void destroyScene(Scene* scene)
{
    if (!scene) return;
    std::lock_guard<std::mutex> lock(gDestroySceneMtx);
    auto it = gPhysXSceneMap.find(scene);
    if (it != gPhysXSceneMap.end())
    {
        gPhysXSceneMap.erase(it);
    }
    scene->shutdown();
    delete scene;
}