#include "phynoProcessEngine.h"
#include "phynoScene.h"
#include "phynoPhysxCallbacks.h"

#include "Poco/LogStream.h"
#include <Poco/JSON/JSON.h>

extern std::map<std::string, physx::PxScene *> Scenes;
extern PxPhysics *gPhysics;
extern PxDefaultCpuDispatcher *gDispatcher;
extern PxMaterial *gMaterial;

extern runningPhysXScenesType runningScenes;

std::string getParamsKey(std::map<std::string, std::string> &params, std::string key)
{
	try
	{
		return (params.at(key));
	}
	// catch (std::exception e) // copy-initialization from the std::exception base
	// {
	//     std::cout << e.what(); // information from length_error is lost
	// }
	catch (const std::exception &e) // reference to the base of a polymorphic object
	{
		Poco::Logger *logger = &Logger::get("PhynoMainLogger");
		logger->error("Key not found : %s   %s", key, e.what());
		return (NULL);
	}
}

phynoEvent *taskPhynoRootMsg(mqttEvent *e)
{
	delete (e);
	return new (phynoEvent);
}

phynoEvent *taskCreateScene(mqttEvent *e)
{
	Poco::Logger *logger = &Logger::get("PhynoMainLogger");
	logger->information("Task create scene : %?s", getParamsKey(e->paramParsed, "scene_name"));
	PxScene *mScene;
	if (gPhysics == 0) {logger->error("NOPHYSICS"); return(NULL);}
	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	logger->information("Screne created");
	// e->paramParsed
	mScene = gPhysics->createScene(sceneDesc);
	phynoScene *pScene = new phynoScene(getParamsKey(e->paramParsed, "scene_name"), mScene);
	mScene->userData = pScene;
	mScene->setSimulationEventCallback(new ephysx_simulation_callbacks());

	Scenes.insert(std::pair<std::string, physx::PxScene *>(getParamsKey(e->paramParsed, "scene_name"), mScene));
	runningPhysXScenesType::accessor ac;
	 runningScenes.insert( ac, std::pair<std::string, PxScene*>(getParamsKey(e->paramParsed, "scene_name"), mScene) );
	delete (e);
	return (new phynoEventCreateScene(getParamsKey(e->paramParsed, "scene_name")));
}

PxRigidDynamic *createDynamic(const PxTransform &t, const PxGeometry &geometry, const PxVec3 &velocity = PxVec3(0))
{
	PxRigidDynamic *dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
	dynamic->setRigidBodyFlags( PxRigidBodyFlag::eENABLE_POSE_INTEGRATION_PREVIEW );
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	return dynamic;
}

phynoEvent *taskCreateEntity(mqttEvent *e)
{
	Poco::Logger *logger = &Logger::get("PhynoMainLogger");
	logger->information("Task create Entity : scene : %?s   Entity: %?s", getParamsKey(e->paramParsed, "scene_name"), getParamsKey(e->paramParsed, "entity_name"));
	PxScene *mScene = Scenes[getParamsKey(e->paramParsed, "scene_name")];

	if (mScene == NULL)
	{
		logger->error("Scene : %?s not found while adding rigid dynamic body: %?s", getParamsKey(e->paramParsed, "scene_name"), getParamsKey(e->paramParsed, "entity_name"));
		return (NULL);
	}
	phynoEventAddDynamicRigid *phynoEvent = new phynoEventAddDynamicRigid(getParamsKey(e->paramParsed, "scene_name"),  getParamsKey(e->paramParsed, "entity_name"));

	phynoEvent->rigidDynamics = createDynamic(PxTransform(PxVec3(0, 100, 0)), PxSphereGeometry(10), PxVec3(0, 0, 0));
	phynoEvent->rigidDynamics->userData = new std::string(getParamsKey(e->paramParsed, "entity_name"));
	mScene->addActor(*phynoEvent->rigidDynamics );
	delete (e);
	return (phynoEvent);
}

mqttPreProcessor::mqttPreProcessor(const std::string &str) : phynoPrefix(str)
{
	logger = &Logger::get("PhynoMainLogger");
	// Initialize multifunction node which is graph entry node
	mqttEventTaskSelector = new taskMqttEventMultiFunctionNode_t(graph, oneapi::tbb::flow::unlimited, mqttEventTaskSelectorBody(str));

	taskMqttEventFunctionNode_t *mqttEventTask;

	mqttEventTask = new taskMqttEventFunctionNode_t(graph, oneapi::tbb::flow::unlimited, taskPhynoRootMsg);
	oneapi::tbb::flow::make_edge(oneapi::tbb::flow::output_port<0>(*mqttEventTaskSelector), *mqttEventTask);
	vectorTasks.push_back(mqttEventTask);

	mqttEventTask = new taskMqttEventFunctionNode_t(graph, oneapi::tbb::flow::unlimited, taskCreateScene);
	oneapi::tbb::flow::make_edge(oneapi::tbb::flow::output_port<2>(*mqttEventTaskSelector), *mqttEventTask);
	vectorTasks.push_back(mqttEventTask);

	mqttEventTask = new taskMqttEventFunctionNode_t(graph, oneapi::tbb::flow::unlimited, taskCreateEntity);
	oneapi::tbb::flow::make_edge(oneapi::tbb::flow::output_port<4>(*mqttEventTaskSelector), *mqttEventTask);
	vectorTasks.push_back(mqttEventTask);
}

mqttPreProcessor::~mqttPreProcessor()
{
}

void mqttPreProcessor::processMqttEvent(mqttEvent *mqttEvent)
{
	mqttEventTaskSelector->try_put(mqttEvent);
}
