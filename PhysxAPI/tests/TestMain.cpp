// Minimal runtime test for CLion.

#include <iostream>

#include "PhysxApi.h"
#include "object/SceneStaticObj.h"
#include "object/SceneRigidObj.h"
#include "object/SceneKinematicObj.h"
#include "object/SceneCharacterObj.h"

int main()
{
    constexpr uint32_t LAYER_GROUND = 1u << 0;
    constexpr uint32_t LAYER_RIGID = 1u << 1;
    constexpr uint32_t LAYER_BLOCKED_GROUND = 1u << 2;
    constexpr uint32_t LAYER_FALLTHROUGH = 1u << 3;

    initPhysxApi();

    Scene* scene = createScene(1.0f / 60.0f);
    if (!scene)
    {
        std::cerr << "createScene failed\n";
        return 1;
    }

    // Ground
    auto* ground = scene->createStaticObject();
    auto* groundStatic = dynamic_cast<SceneStaticObj*>(ground);
    if (groundStatic && groundStatic->primaryActorRecord())
    {
        groundStatic->primaryActorRecord()->setCollisionFilter(LAYER_GROUND, LAYER_RIGID);
        auto* shape = groundStatic->primaryActorRecord()->createBoxShape(physx::PxVec3(0.5f, 0.5f, 0.5f));
        groundStatic->attachShape(shape);
        physx::PxVec3 p(0, -1, 0);
        groundStatic->teleport(&p);
    }

    // Dynamic box
    auto* rigid = scene->createRigidObject();
    auto* rigidObj = dynamic_cast<SceneRigidObj*>(rigid);
    if (rigidObj && rigidObj->primaryActorRecord())
    {
        rigidObj->primaryActorRecord()->setCollisionFilter(LAYER_RIGID, LAYER_GROUND | LAYER_BLOCKED_GROUND);
        auto* shape = rigidObj->primaryActorRecord()->createBoxShape(physx::PxVec3(0.5f, 0.5f, 0.5f));
        rigidObj->attachShape(shape);
        physx::PxVec3 p(0, 5, 0);
        rigidObj->teleport(&p);
    }

    // Ground that should be ignored by another rigid due to layer/mask mismatch.
    auto* blockedGround = scene->createStaticObject();
    auto* blockedGroundObj = dynamic_cast<SceneStaticObj*>(blockedGround);
    if (blockedGroundObj && blockedGroundObj->primaryActorRecord())
    {
        blockedGroundObj->primaryActorRecord()->setCollisionFilter(LAYER_BLOCKED_GROUND, LAYER_BLOCKED_GROUND);
        auto* shape = blockedGroundObj->primaryActorRecord()->createBoxShape(physx::PxVec3(0.5f, 0.5f, 0.5f));
        blockedGroundObj->attachShape(shape);
        physx::PxVec3 p(2, -1, 0);
        blockedGroundObj->teleport(&p);
    }

    auto* fallthroughRigid = scene->createRigidObject();
    auto* fallthroughRigidObj = dynamic_cast<SceneRigidObj*>(fallthroughRigid);
    if (fallthroughRigidObj && fallthroughRigidObj->primaryActorRecord())
    {
        fallthroughRigidObj->primaryActorRecord()->setCollisionFilter(LAYER_FALLTHROUGH, LAYER_FALLTHROUGH);
        auto* shape = fallthroughRigidObj->primaryActorRecord()->createBoxShape(physx::PxVec3(0.5f, 0.5f, 0.5f));
        fallthroughRigidObj->attachShape(shape);
        physx::PxVec3 p(2, 5, 0);
        fallthroughRigidObj->teleport(&p);
    }

    // Kinematic (just to ensure it creates)
    auto* kinematic = scene->createKinematicObject();
    auto* kinematicObj = dynamic_cast<SceneKinematicObj*>(kinematic);
    if (kinematicObj && kinematicObj->primaryActorRecord())
    {
        kinematicObj->primaryActorRecord()->setCollisionFilter(LAYER_RIGID, LAYER_GROUND);
    }

    // Trigger role attached to a static movement object.
    auto* trigger = scene->createStaticObject();
    auto* triggerObj = dynamic_cast<SceneStaticObj*>(trigger);
    if (triggerObj && triggerObj->primaryActorRecord())
    {
        auto* shape = triggerObj->primaryActorRecord()->createBoxShape(physx::PxVec3(0.5f, 0.5f, 0.5f));
        triggerObj->addTriggerShape(shape);
        physx::PxVec3 p(0, 1, 0);
        triggerObj->teleport(&p);
    }

    auto* character = scene->createCharacterObject(0.3f, 1.0f, physx::PxExtendedVec3(4.0, 2.0, 0.0));
    auto* characterObj = dynamic_cast<SceneCharacterObj*>(character);
    if (characterObj && characterObj->primaryActorRecord())
    {
        characterObj->primaryActorRecord()->setCollisionFilter(LAYER_RIGID, LAYER_GROUND);
        auto* triggerShape = characterObj->primaryActorRecord()->createBoxShape(physx::PxVec3(0.2f, 0.2f, 0.2f));
        characterObj->addHitboxShape(triggerShape, physx::PxTransform(physx::PxVec3(0.0f, 0.5f, 0.0f)));
    }

    auto* characterWall = scene->createStaticObject();
    auto* characterWallObj = dynamic_cast<SceneStaticObj*>(characterWall);
    if (characterWallObj && characterWallObj->primaryActorRecord())
    {
        characterWallObj->primaryActorRecord()->setCollisionFilter(LAYER_GROUND, LAYER_RIGID);
        auto* shape = characterWallObj->primaryActorRecord()->createBoxShape(physx::PxVec3(0.2f, 1.0f, 1.0f));
        characterWallObj->attachShape(shape);
        physx::PxVec3 p(4.8f, 0.5f, 0.0f);
        characterWallObj->teleport(&p);
    }

    bool sawContactFound = false;
    bool sawContactPersist = false;
    bool sawTriggerEnter = false;
    bool sawTriggerPersist = false;
    bool sawTriggerExit = false;
    bool sawCharacterShapeHit = false;

    for (int i = 0; i < 120; ++i)
    {
        if (i == 80 && triggerObj)
        {
            physx::PxVec3 movedPos(5, 1, 0);
            triggerObj->teleport(&movedPos);
        }

        if (characterObj)
        {
            physx::PxVec3 movement(0.05f, 0.0f, 0.0f);
            characterObj->move(&movement);
        }

        updateScene(scene);

        for (const SceneContactEvent& event : scene->contactEvents())
        {
            if (event.objectAHandle == 0 || event.objectBHandle == 0)
            {
                std::cerr << "contact event missing object handle\n";
                destroyScene(scene);
                shutdownPhysxApi();
                return 14;
            }
            if (event.type == SceneContactEvent::Type::FOUND) sawContactFound = true;
            if (event.type == SceneContactEvent::Type::PERSIST) sawContactPersist = true;
        }

        for (const SceneTriggerEvent& event : scene->triggerEvents())
        {
            if (event.triggerHandle == 0 || event.otherHandle == 0)
            {
                std::cerr << "trigger event missing object handle\n";
                destroyScene(scene);
                shutdownPhysxApi();
                return 15;
            }
            if (event.type == SceneTriggerEvent::Type::ENTER) sawTriggerEnter = true;
            if (event.type == SceneTriggerEvent::Type::PERSIST) sawTriggerPersist = true;
            if (event.type == SceneTriggerEvent::Type::EXIT) sawTriggerExit = true;
        }

        for (const SceneCharacterHitEvent& event : scene->characterHitEvents())
        {
            if (event.controllerHandle == 0)
            {
                std::cerr << "character hit event missing controller handle\n";
                destroyScene(scene);
                shutdownPhysxApi();
                return 18;
            }
            if (event.type == SceneCharacterHitEvent::Type::SHAPE_HIT)
            {
                if (event.otherHandle == 0)
                {
                    std::cerr << "character shape hit missing other handle\n";
                    destroyScene(scene);
                    shutdownPhysxApi();
                    return 19;
                }
                sawCharacterShapeHit = true;
            }
        }

        if (rigidObj && rigidObj->actor())
        {
            const auto pose = rigidObj->actor()->getGlobalPose();
            if (i % 30 == 0)
            {
                std::cout << "frame=" << i << " rigid.y=" << pose.p.y << "\n";
            }
        }
    }

    if (!rigidObj || !rigidObj->actor())
    {
        std::cerr << "rigid object missing\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 2;
    }
    if (!fallthroughRigidObj || !fallthroughRigidObj->actor())
    {
        std::cerr << "fallthrough rigid object missing\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 3;
    }
    if (!characterObj || !characterObj->controller())
    {
        std::cerr << "character object missing\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 4;
    }

    const float landedY = rigidObj->actor()->getGlobalPose().p.y;
    const float fallthroughY = fallthroughRigidObj->actor()->getGlobalPose().p.y;
    std::cout << "landedRigid.y=" << landedY << " fallthroughRigid.y=" << fallthroughY << "\n";
    if (landedY < -0.2f)
    {
        std::cerr << "collision filter failed: allowed rigid did not land on ground\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 5;
    }
    if (fallthroughY > -1.0f)
    {
        std::cerr << "collision filter failed: blocked rigid unexpectedly collided with ground\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 6;
    }
    if (!sawContactFound || !sawContactPersist)
    {
        std::cerr << "contact callback failed: expected found and persist events\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 16;
    }
    if (!sawTriggerEnter || !sawTriggerPersist || !sawTriggerExit)
    {
        std::cerr << "trigger callback failed: expected enter/persist/exit events\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 17;
    }
    if (!sawCharacterShapeHit)
    {
        std::cerr << "character hit callback failed: expected shape hit events\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 20;
    }

    const QueryResult hitGround = scene->rayCast(physx::PxVec3(0, 2, 0), physx::PxVec3(0, -1, 0), 10.0f, LAYER_GROUND);
    if (!hitGround.hasHit)
    {
        std::cerr << "query filter failed: expected ground hit for ground layer mask\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 7;
    }
    if (hitGround.objectHandle == 0)
    {
        std::cerr << "query result missing object handle for raycast hit\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 21;
    }

    const QueryResult hitBlockedGround = scene->rayCast(physx::PxVec3(2, 2, 0), physx::PxVec3(0, -1, 0), 10.0f, LAYER_BLOCKED_GROUND);
    if (!hitBlockedGround.hasHit)
    {
        std::cerr << "query filter failed: expected blocked ground hit for blocked-ground layer mask\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 8;
    }

    const QueryResult missGround = scene->rayCast(physx::PxVec3(0, 2, 0), physx::PxVec3(0, -1, 0), 10.0f, LAYER_FALLTHROUGH);
    if (missGround.hasHit)
    {
        std::cerr << "query filter failed: unexpected hit for fallthrough-only query mask at origin x=0\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 9;
    }

    const physx::PxBoxGeometry queryBox(0.25f, 0.25f, 0.25f);
    const QueryResult sweepGround = scene->sweep(queryBox,
                                                 physx::PxTransform(physx::PxVec3(0, 2, 0)),
                                                 physx::PxVec3(0, -1, 0),
                                                 10.0f,
                                                 LAYER_GROUND);
    if (!sweepGround.hasHit)
    {
        std::cerr << "query filter failed: expected sweep hit for ground layer mask\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 10;
    }

    const QueryResult sweepMiss = scene->sweep(queryBox,
                                               physx::PxTransform(physx::PxVec3(0, 2, 0)),
                                               physx::PxVec3(0, -1, 0),
                                               10.0f,
                                               LAYER_FALLTHROUGH);
    if (sweepMiss.hasHit)
    {
        std::cerr << "query filter failed: unexpected sweep hit for fallthrough-only mask\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 11;
    }

    const QueryResult overlapGround = scene->overlap(queryBox,
                                                     physx::PxTransform(physx::PxVec3(0, -1, 0)),
                                                     LAYER_GROUND);
    if (!overlapGround.hasHit)
    {
        std::cerr << "query filter failed: expected overlap hit for ground layer mask\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 12;
    }

    const QueryResult overlapMiss = scene->overlap(queryBox,
                                                   physx::PxTransform(physx::PxVec3(0, -1, 0)),
                                                   LAYER_FALLTHROUGH);
    if (overlapMiss.hasHit)
    {
        std::cerr << "query filter failed: unexpected overlap hit for fallthrough-only mask\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 13;
    }

    const std::vector<RayCastRequest> rayBatchRequests = {
        RayCastRequest(physx::PxVec3(0, 2, 0), physx::PxVec3(0, -1, 0), 10.0f, LAYER_GROUND),
        RayCastRequest(physx::PxVec3(0, 2, 0), physx::PxVec3(0, -1, 0), 10.0f, LAYER_FALLTHROUGH),
    };
    const QueryBatchResult rayBatchResult = scene->rayCastBatch(rayBatchRequests);
    if (rayBatchResult.size() != rayBatchRequests.size())
    {
        std::cerr << "ray batch result size mismatch\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 22;
    }
    if (rayBatchResult[0].hasHit != hitGround.hasHit ||
        rayBatchResult[0].objectHandle != hitGround.objectHandle ||
        rayBatchResult[1].hasHit != missGround.hasHit)
    {
        std::cerr << "ray batch result inconsistent with single query\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 23;
    }

    const std::vector<SweepRequest> sweepBatchRequests = {
        SweepRequest(queryBox, physx::PxTransform(physx::PxVec3(0, 2, 0)), physx::PxVec3(0, -1, 0), 10.0f, LAYER_GROUND),
        SweepRequest(queryBox, physx::PxTransform(physx::PxVec3(0, 2, 0)), physx::PxVec3(0, -1, 0), 10.0f, LAYER_FALLTHROUGH),
    };
    const QueryBatchResult sweepBatchResult = scene->sweepBatch(sweepBatchRequests);
    if (sweepBatchResult.size() != sweepBatchRequests.size())
    {
        std::cerr << "sweep batch result size mismatch\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 24;
    }
    if (sweepBatchResult[0].hasHit != sweepGround.hasHit ||
        sweepBatchResult[0].objectHandle != sweepGround.objectHandle ||
        sweepBatchResult[1].hasHit != sweepMiss.hasHit)
    {
        std::cerr << "sweep batch result inconsistent with single query\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 25;
    }

    const std::vector<OverlapRequest> overlapBatchRequests = {
        OverlapRequest(queryBox, physx::PxTransform(physx::PxVec3(0, -1, 0)), LAYER_GROUND),
        OverlapRequest(queryBox, physx::PxTransform(physx::PxVec3(0, -1, 0)), LAYER_FALLTHROUGH),
    };
    const QueryBatchResult overlapBatchResult = scene->overlapBatch(overlapBatchRequests);
    if (overlapBatchResult.size() != overlapBatchRequests.size())
    {
        std::cerr << "overlap batch result size mismatch\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 26;
    }
    if (overlapBatchResult[0].hasHit != overlapGround.hasHit ||
        overlapBatchResult[0].objectHandle != overlapGround.objectHandle ||
        overlapBatchResult[1].hasHit != overlapMiss.hasHit)
    {
        std::cerr << "overlap batch result inconsistent with single query\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 27;
    }
    if (rayBatchResult.hitCount() != 1 || sweepBatchResult.hitCount() != 1 || overlapBatchResult.hitCount() != 1)
    {
        std::cerr << "query batch hit counts are incorrect\n";
        destroyScene(scene);
        shutdownPhysxApi();
        return 28;
    }

    destroyScene(scene);
    shutdownPhysxApi();
    std::cout << "OK\n";
    return 0;
}

