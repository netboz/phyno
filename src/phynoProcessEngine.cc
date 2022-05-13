#include "phynoProcessEngine.h"
#include "phynoScene.h"
#include "phynoPhysxCallbacks.h"

#include "Poco/LogStream.h"
#include <Poco/JSON/JSON.h>
#include "tbb/task.h"  

#include "tbb/task_scheduler_observer.h"  


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
	catch (const std::exception &e) // reference to the base of a polymorphic object
	{
		Poco::Logger *logger = &Logger::get("PhynoMainLogger");
		logger->error("MQTT_PROCESSOR: Key not found : %s   %s", key, e.what());
		return (NULL);
	}
}

void taskPhynoRootMsg(mqttEvent *e)
{
	delete (e);
}

void taskCreateScene(mqttEvent *e)
{
	Poco::Logger *logger = &Logger::get("PhynoMainLogger");
	logger->information("MQTT_PROCESSOR: Task create scene : %?s", getParamsKey(e->paramParsed, "scene_name"));
	PxScene *mScene;
	if (gPhysics == 0)
	{
		logger->error("MQTT_PROCESSOR: Physx isn't initialised");
		return;
	}
	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	std::string sceneName = std::string(getParamsKey(e->paramParsed, "scene_name"));

	logger->information("MQTT_PROCESSOR: created scene :%s", sceneName);
	// e->paramParsed
	mScene = gPhysics->createScene(sceneDesc);

	phynoScene *pScene = new phynoScene(sceneName, mScene);
	mScene->userData = pScene;
	mScene->setSimulationEventCallback(new ephysx_simulation_callbacks());

	Scenes.insert(std::pair<std::string, physx::PxScene *>(sceneName, mScene));
	runningPhysXScenesType::accessor ac;
	runningScenes.insert(ac, std::pair<std::string, PxScene *>(sceneName, mScene));
	delete (e);
}

PxRigidDynamic *createDynamic(const PxTransform &t, const PxGeometry &geometry, const PxVec3 &velocity = PxVec3(0))
{
	PxRigidDynamic *dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
	dynamic->setRigidBodyFlags(PxRigidBodyFlag::eENABLE_POSE_INTEGRATION_PREVIEW);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	return dynamic;
}

void taskCreateEntity(mqttEvent *e)
{
	Poco::Logger *logger = &Logger::get("PhynoMainLogger");
	std::string scene_name = getParamsKey(e->paramParsed, "scene_name");
	std::string entity_name = getParamsKey(e->paramParsed, "entity_name");
	logger->information("MQTT_PROCESSOR: Task create Entity : scene : %?s   Entity: %?s", scene_name, entity_name);
	PxScene *mScene = Scenes[scene_name];

	if (mScene == NULL)
	{
		logger->error("MQTT_PROCESSOR: Scene : %?s not found while adding rigid dynamic body: %?s", scene_name, entity_name);
		return;
	}
	phynoEventAddDynamicRigid *phynoEvent = new phynoEventAddDynamicRigid(scene_name, entity_name);

	phynoEvent->rigidDynamics = createDynamic(PxTransform(PxVec3(0, 100, 0)), PxSphereGeometry(10), PxVec3(0, 0, 0));
	phynoEvent->rigidDynamics->userData = new std::string(entity_name);
	phynoEvent->targetScene=mScene;
	((phynoScene *)(mScene->userData))->sceneEventQueue.push(phynoEvent);
	delete (e);
}

mqttPreProcessor::mqttPreProcessor(const std::string &str) : phynoPrefix(str)
{
	logger = &Logger::get("PhynoMainLogger");
	// Initialize multifunction node which is graph entry node
	mqttEventTaskSelector = new taskMqttEventMultiFunctionNode_t(graph, oneapi::tbb::flow::unlimited, mqttEventTaskSelectorBody(str));
	 int nDefThreads = oneapi::tbb::info::default_concurrency();
	 int max = oneapi::tbb::this_task_arena::max_concurrency() ;
	 int gt = oneapi::tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism) ;
	logger->information("MQTT_PROCESSOR: NB Available threads : default :%d    Max:%d      thread pool limit:%d", nDefThreads, max, gt);
	taskMqttEventFunctionNode_t *mqttEventTask;

	auto task1 = std::make_unique<taskMqttEventFunctionNode_t>(graph, oneapi::tbb::flow::unlimited, taskPhynoRootMsg);
	oneapi::tbb::flow::make_edge(oneapi::tbb::flow::output_port<0>(*mqttEventTaskSelector), *task1);
	vectorTasks.push_back(std::move(task1));

	auto task2 = std::make_unique<taskMqttEventFunctionNode_t>(graph, oneapi::tbb::flow::unlimited, taskCreateScene);
	oneapi::tbb::flow::make_edge(oneapi::tbb::flow::output_port<2>(*mqttEventTaskSelector), *task2);
	vectorTasks.push_back(std::move(task2));

	auto task3 = std::make_unique<taskMqttEventFunctionNode_t>(graph, oneapi::tbb::flow::unlimited, taskCreateEntity);
	oneapi::tbb::flow::make_edge(oneapi::tbb::flow::output_port<4>(*mqttEventTaskSelector), *task3);
	vectorTasks.push_back(std::move(task3));

}

mqttPreProcessor::~mqttPreProcessor()
{
	delete(mqttEventTaskSelector);
}

void mqttPreProcessor::processMqttEvent(mqttEvent *mqttEvent)
{
	mqttEventTaskSelector->try_put(mqttEvent);
}
