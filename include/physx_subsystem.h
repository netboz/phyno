#pragma once

#include "Poco/Util/Application.h"
#include "Poco/Util/Subsystem.h"
#include "Poco/Logger.h"
#include "Poco/Timer.h"
#include "Poco/Thread.h"

#include "PxPhysicsAPI.h"
#include "PxSimpleFactory.h"
#include "task/PxCpuDispatcher.h"
#include "extensions/PxDefaultAllocator.h"

#include "tbb/concurrent_hash_map.h"

using Poco::Timer;
using Poco::TimerCallback;
using Poco::Util::Subsystem;
using namespace physx;

#define PVD_HOST "192.168.178.144"

typedef tbb::concurrent_hash_map<std::string, PxScene *> runningPhysXScenesType;

// parallel_for Body
class phynoScene;

class callbackTimerClass
{
public:
	const uint16_t stepSize = 1000/60;
	void onTimer(Poco::Timer &timer);
	
};

class physx_subsystem : public Subsystem, public physx::PxCpuDispatcher
{
public:
	void runSimulations();
	physx_subsystem(void);
	~physx_subsystem(void);

	bool simulationsRunning = false;
	callbackTimerClass callbackTimer;

	static const uint16_t step = 1000 / 60;

	Poco::Util::Application *self_app;
	void submitTask(PxBaseTask &task);
	uint32_t getWorkerCount() const;

	Timer *timer;


protected:
	void initialize(Poco::Util::Application &app);
	void reinitialize(Poco::Util::Application &app);
	void uninitialize();

	void defineOptions(Poco::Util::OptionSet &options);
	virtual const char *name() const;

private:
};
