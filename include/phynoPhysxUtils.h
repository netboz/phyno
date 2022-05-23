#pragma once

#include "yyjson.h"

#include "PxPhysicsAPI.h"

using namespace physx;


char json2vec3(yyjson_val *, PxVec3 *);

char json2Quat(yyjson_val *, PxQuat *);

char json2Transform(yyjson_val *, PxTransform *);

void dump_vect(PxVec3 v);
