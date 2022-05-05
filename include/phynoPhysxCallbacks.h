#pragma once

#include "PxPhysicsAPI.h"

using namespace physx;

class ephysx_simulation_callbacks : public physx::PxSimulationEventCallback
    {
    void 	onConstraintBreak (PxConstraintInfo *constraints, PxU32 count);
    void 	onWake (PxActor **actors, PxU32 count);
    void 	onSleep (PxActor **actors, PxU32 count);
    void 	onContact (const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 nbPairs);
    void 	onTrigger (PxTriggerPair *pairs, PxU32 count);
    void 	onAdvance (const PxRigidBody *const *bodyBuffer, const PxTransform *poseBuffer, const PxU32 count);
    };
