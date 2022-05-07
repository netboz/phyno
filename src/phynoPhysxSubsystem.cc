#include "phynoPhysxSubsystem.h"
#include "phynoScene.h"

#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>

#include "Poco/String.h"
#include "Poco/HashMap.h"

#include <tbb/parallel_for.h>

// access to Physx Globals

PxDefaultAllocator gAllocator;
PxDefaultErrorCallback gErrorCallback;
PxFoundation *gFoundation;
PxPvd *gPvd;
PxMaterial *gMaterial;
PxPhysics *gPhysics;
PxDefaultCpuDispatcher *gDispatcher;
std::map<std::string, PxScene *> Scenes;

runningPhysXScenesType runningScenes;

void callbackTimerClass::onTimer(Poco::Timer & /*timer*/)
{
    runningPhysXScenesType::range_type r = runningScenes.range();
    // Request each running scene to simulate a step
    tbb::parallel_for(
        r,
        [this](runningPhysXScenesType::range_type r)
        {	
				for (runningPhysXScenesType::iterator i = r.begin(); i != r.end(); ++i)
					{
					PxScene *sc = i->second;
					phynoScene *psc = (phynoScene*)sc->userData;
					psc->step(stepSize);	
					} },
        tbb::auto_partitioner());
}

physx_subsystem::physx_subsystem(void)
{
}

physx_subsystem::~physx_subsystem(void)
{
}

void physx_subsystem::initialize(Poco::Util::Application &app)
{
    self_app = &app;
    app.logger().information("Initializing Physx-Subsystem");
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

    if (!gFoundation)
        app.logger().error("Error initializing PhysX foundations.");

    gPvd = PxCreatePvd(*gFoundation);
    PxPvdTransport *transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    if (!gPhysics)
        app.logger().error("Error initializing PhysX engine.");
    // Initialize physx task dispatcher so it uses calling api thread
    gDispatcher = PxDefaultCpuDispatcherCreate(0);
    runSimulations();
}

void physx_subsystem::reinitialize(Poco::Util::Application &app)
{
    app.logger().information("Re-initializing MQTT-Subsystem");
    uninitialize();
    initialize(app);
}

void physx_subsystem::uninitialize()
{
    self_app->logger().information("UN-Initializing MQTT-Subsystem");
    gMaterial->release();
    gPhysics->release();
    gFoundation->release();
    gDispatcher->release();
}

void physx_subsystem::defineOptions(Poco::Util::OptionSet &options)
{
    Subsystem::defineOptions(options);
    options.addOption(
        Poco::Util::Option("mhelp", "mh", "Display help about PHYSX Subsystem")
            .required(false)
            .repeatable(false));
}

const char *physx_subsystem::name() const
{
    return "PHYSX-Subsystem";
}

void physx_subsystem::submitTask(PxBaseTask &/*task*/)
{
}

uint32_t physx_subsystem::getWorkerCount() const
{
    return (32);
}

void physx_subsystem::runSimulations()
{
    Poco::Logger *logger = &Logger::get("PhynoMainLogger");
    logger->information("Starting simulation");
    TimerCallback<callbackTimerClass> callback(callbackTimer, &callbackTimerClass::onTimer);
    timer = new Timer(1, 16);
    simulationsRunning = true;
    timer->start(callback);
}