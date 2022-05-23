
#include "phynoProcessEngine.h"
#include "phynoScene.h"
#include "phynoPhysxCallbacks.h"
#include "phynoPhysxUtils.h"
#include "phynoMqttSubsystem.h"
#include "Poco/LogStream.h"

#include "tbb/task.h"
#include "tbb/task_scheduler_observer.h"

#include <iostream>

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

	yyjson_doc *doc = yyjson_read(e->payload.c_str(), e->payload.length(), 0);

	if (doc == NULL)
	{
		logger->error("Couldn't parse Create Scene message :%s", e->payload);
		return;
	}

	yyjson_val *root = yyjson_doc_get_root(doc);

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());

	PxVec3 gravity(0. - 9, 8, 0);
	yyjson_val *jgrav;
	if ((jgrav = yyjson_obj_get(root, "gravity")) != NULL)
	{
		json2vec3(jgrav, &gravity);
	}

	sceneDesc.gravity = gravity;

	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	std::string sceneName = std::string(getParamsKey(e->paramParsed, "scene_name"));

	logger->information("MQTT_PROCESSOR: created scene :%s", sceneName);

	mScene = gPhysics->createScene(sceneDesc);

	phynoScene *pScene = new phynoScene(sceneName, mScene);
	mScene->userData = pScene;
	mScene->setSimulationEventCallback(new ephysx_simulation_callbacks());

	Scenes.insert(std::pair<std::string, physx::PxScene *>(sceneName, mScene));
	runningPhysXScenesType::accessor ac;
	runningScenes.insert(ac, std::pair<std::string, PxScene *>(sceneName, mScene));
	logger->information("Scene created : %?s", getParamsKey(e->paramParsed, "scene_name"));

	mqtt_subsystem &mqttSub = Poco::Util::Application::instance().getSubsystem<mqtt_subsystem>();
	std::string topic = "/scenes/" + pScene->name + "/status";
	std::string status("created");
	mqttSub.send(topic, status.c_str(), status.length(), true);

	yyjson_doc_free(doc);

	delete (e);
}

PxRigidDynamic *createDynamic(float mass, const PxTransform &t, const PxGeometry &geometry, const PxVec3 &velocity = PxVec3(0))
{
	PxRigidDynamic *dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, mass);
	dynamic->setRigidBodyFlags(PxRigidBodyFlag::eENABLE_POSE_INTEGRATION_PREVIEW);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	return dynamic;
}

void taskCreateEntity(mqttEvent *e)
{
	Poco::Logger *logger = &Logger::get("PhynoMainLogger");
	std::string scene_name = getParamsKey(e->paramParsed, "scene_name");
	std::string actor_name = getParamsKey(e->paramParsed, "actor_name");
	logger->information("MQTT_PROCESSOR: Task create Entity : scene : %?s   Entity: %?s", scene_name, actor_name);

	PxScene *mScene = Scenes[scene_name];

	if (mScene == NULL)
	{
		logger->error("MQTT_PROCESSOR: Scene : %?s not found while adding rigid dynamic body: %?s", scene_name, actor_name);
		return;
	}
	logger->information("parsing");

	yyjson_doc *doc = yyjson_read(e->payload.c_str(), e->payload.length(), 0);

	if (doc == NULL)
	{
		logger->error("Couldn't parse Create Scene message :%s", std::string(e->payload, e->payload.length()));
		return;
	}

	yyjson_val *root = yyjson_doc_get_root(doc);

	double mass = 1;

	if (yyjson_val *jmass = yyjson_obj_get(root, "mass"))
	{
		mass = yyjson_get_real(jmass);
	}

	logger->information("parsed mass");

	PxVec3 velocity;

	yyjson_val *jVelocity = yyjson_get_pointer(root, "/rigidActor/velocity");

	if (jVelocity)
	{
		if (json2vec3(jVelocity, &velocity))
		{
			logger->warning("MQTT_PROCESSOR: Failled to decode velocity for dynamic rigid :%s", actor_name);
			return;
		}
	}

	logger->information("parsed velo");

	PxTransform pose(PxVec3(0, 0, 0));
	yyjson_val *jpose = yyjson_get_pointer(root, "/rigidActor/pose");

	if (!jpose)
	{
		if (json2Transform(jpose, &pose))
		{
			logger->warning("MQTT_PROCESSOR: Failled to decode pose for dynamic rigid :%s", actor_name);
			return;
		}
	}
	logger->information("Got transform");

	phynoEventAddDynamicRigid *phynoEvent = new phynoEventAddDynamicRigid(scene_name, actor_name);

	phynoEvent->rigidDynamics = createDynamic(mass, PxTransform(PxVec3(1, 2, 3)), PxSphereGeometry(10), velocity);
	phynoEvent->rigidDynamics->userData = new std::string(actor_name);
	phynoEvent->targetScene = mScene;
	((phynoScene *)(mScene->userData))->sceneEventQueue.push(phynoEvent);
	logger->information("actor created");

	mqtt_subsystem &mqttSub = Poco::Util::Application::instance().getSubsystem<mqtt_subsystem>();
	std::string topic = "/scenes/" +scene_name + "/status";
	std::string status("created");
	mqttSub.send(topic, status.c_str(), status.length(), true);

	yyjson_doc_free(doc);
	delete (e);
}

mqttPreProcessor::mqttPreProcessor(const std::string &str) : phynoPrefix(str)
{
	logger = &Logger::get("PhynoMainLogger");
	// Initialize multifunction node which is graph entry node
	mqttEventTaskSelector = new taskMqttEventMultiFunctionNode_t(graph, oneapi::tbb::flow::unlimited, mqttEventTaskSelectorBody(str));
	int nDefThreads = oneapi::tbb::info::default_concurrency();
	int max = oneapi::tbb::this_task_arena::max_concurrency();
	int gt = oneapi::tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
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
	delete (mqttEventTaskSelector);
}

void mqttPreProcessor::processMqttEvent(mqttEvent *mqttEvent)
{
	mqttEventTaskSelector->try_put(mqttEvent);
}
