#include "phynoProcessEngine.h"
#include "Poco/LogStream.h"
#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Parser.h>
#include "PxPhysicsAPI.h"

using namespace physx;


extern std::map<std::string, physx::PxScene*> Scenes;
extern PxPhysics*gPhysics;
extern PxDefaultCpuDispatcher* gDispatcher;

phynoEvent *taskPhynoRootMsg(mqttEvent *e) {
	Poco::Logger *logger = &Logger::get("PhynoMainLogger");
	logger->information("Task phyno root message");
	return new(phynoEvent);
}

phynoEvent *taskCreateScene(mqttEvent *) {
	Poco::Logger *logger = &Logger::get("PhynoMainLogger");
	logger->information("Task create scene");
	PxScene* mScene;

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);

	sceneDesc.cpuDispatcher    = gDispatcher;

	mScene = gPhysics->createScene(sceneDesc);
	Scenes.insert(std::pair<std::string, physx::PxScene*>("test", mScene));

	return new(phynoEvent);
}

phynoEvent *taskCreateEntity(mqttEvent *) {return new(phynoEvent);}
phynoEvent *taskCreateModel(mqttEvent *) {return new(phynoEvent);}

mqttPreProcessor::mqttPreProcessor(const std::string& str)  : phynoPrefix(str)
{
	logger = &Logger::get("PhynoMainLogger");
	// Initialize multifunction node which is graph entry node
	mqttEventTaskSelector = new taskMqttEventMultiFunctionNode_t(graph, oneapi::tbb::flow::unlimited, mqttEventTaskSelectorBody(str));

	taskMqttEventFunctionNode_t *mqttEventTask;


	mqttEventTask = new taskMqttEventFunctionNode_t(graph, oneapi::tbb::flow::unlimited, taskPhynoRootMsg);
	oneapi::tbb::flow::make_edge(oneapi::tbb::flow::output_port<0>(*mqttEventTaskSelector), *mqttEventTask);
	vectorTasks.push_back(mqttEventTask);

	mqttEventTask = new taskMqttEventFunctionNode_t(graph, oneapi::tbb::flow::unlimited, taskCreateScene);
	oneapi::tbb::flow::make_edge(oneapi::tbb::flow::output_port<1>(*mqttEventTaskSelector), *mqttEventTask);
	vectorTasks.push_back(mqttEventTask);
}


mqttPreProcessor::~mqttPreProcessor()
{

}

void mqttPreProcessor::processMqttEvent(mqttEvent* mqttEvent)
{
	mqttEventTaskSelector->try_put(mqttEvent);

}

