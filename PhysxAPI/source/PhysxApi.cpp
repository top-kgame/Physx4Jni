#include "PhysxApi.h"

#include <mutex>
#include <set>

static physx::PxFoundation* gFoundation = nullptr;
static physx::PxDefaultCpuDispatcher* gDispatcher = physx::PxDefaultCpuDispatcherCreate(0);
static physx::PxPhysics *gPhysics = nullptr;
static physx::PxMaterial *gMaterial = nullptr;
static physx::PxPvd *gPvd = nullptr;

std::set<Scene *> gPhysXSceneMap;
std::mutex gCreateSceneMtx;
std::mutex gDestroySceneMtx;

void initPhysxApi()
{
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocatorCallback, gErrorCallback);
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, physx::PxTolerancesScale(), true, gPvd);
    if (!PxInitExtensions(*gPhysics, gPvd))
    {
        gErrorCallback.reportError(physx::PxErrorCode::eABORT, "PxInitExtensions failed!", __FILE__, __LINE__);
    }
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0);
}