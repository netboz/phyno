#pragma once

#include <iostream>
#include <chrono>

#include <PxPhysicsAPI.h>
#include <PxRigidDynamic.h>

using namespace physx;

typedef enum phynoEventTypes
{
	GENERIC,
	CREATE_SCENE,
	NEW_RIGID_DYNAMIC
} phynoEventTypes;

class phynoEvent
{
	phynoEventTypes type;

public:
	phynoEvent(phynoEventTypes typeIn = GENERIC) : type(typeIn) {}

	std::chrono::time_point<std::chrono::system_clock> date;
};

class phynoEventCreateScene : public phynoEvent
{
public:
	phynoEventCreateScene(std::string nameIn) : phynoEvent(CREATE_SCENE), name(nameIn) {}
	std::string name;
};

class phynoEventAddDynamicRigid : public phynoEvent
{
public:
	phynoEventAddDynamicRigid(std::string sceneNameIn, std::string entityNameIn) : phynoEvent(NEW_RIGID_DYNAMIC), sceneName(sceneNameIn), entityName(entityNameIn) {}
	std::string sceneName;
	std::string entityName;
	PxRigidDynamic* rigidDynamics;
};
