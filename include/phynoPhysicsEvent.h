#pragma once

#include <iostream>
#include <chrono>

#include <PxPhysicsAPI.h>
#include <PxRigidDynamic.h>

using namespace physx;

typedef enum phynoEventTypes
{
	GENERIC,
	NO_ACTION,
	CREATE_SCENE,
	NEW_RIGID_DYNAMIC,
} phynoEventTypes;

class phynoEvent
{
	phynoEventTypes type;

public:
	phynoEvent(phynoEventTypes typeIn = GENERIC) : type(typeIn) {}

	std::chrono::time_point<std::chrono::system_clock> date;

	virtual void execute(void) {}
};

class phynoEventNoAction : public phynoEvent
{
public:
	phynoEventNoAction() : phynoEvent(NO_ACTION) {}
	void execute(void) {}
};

class phynoEventCreateScene : public phynoEvent
{
public:
	phynoEventCreateScene(std::string nameIn) : phynoEvent(CREATE_SCENE), sceneName(nameIn) {}
	std::string sceneName;
	PxScene *scene;
	void execute(void);
};

class phynoEventAddDynamicRigid : public phynoEvent
{
public:
	phynoEventAddDynamicRigid(std::string sceneNameIn, std::string entityNameIn) : phynoEvent(NEW_RIGID_DYNAMIC), sceneName(sceneNameIn), entityName(entityNameIn) {}
	std::string sceneName;
	std::string entityName;
	PxScene *targetScene;
	PxRigidDynamic *rigidDynamics;
	void execute(void);
};
