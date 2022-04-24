#include "physx_subsystem.h"
#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>

#include "Poco/String.h"
#include "Poco/HashMap.h"


PxDefaultAllocator              gAllocator;
PxDefaultErrorCallback          gErrorCallback;
PxFoundation*                   gFoundation = NULL;
PxPvd*                          gPvd        = NULL;

PxPhysics*                      gPhysics    = NULL;
PxDefaultCpuDispatcher*         gDispatcher = NULL;
std::map<std::string, PxScene*> Scenes;



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
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);

    if (!gPhysics)
        app.logger().error("Error initializing PhysX engine.");
}


void physx_subsystem::reinitialize(Poco::Util::Application &app)
{
    //mqtt_subsystem::uninitialize();
    //mqtt_subsystem::initialize();
}

void physx_subsystem::uninitialize()
{
    self_app->logger().information("UN-Initializing MQTT-Subsystem");

    gPhysics->release();
    gFoundation->release();
}

void physx_subsystem::defineOptions(Poco::Util::OptionSet& options)
{
    Subsystem::defineOptions(options);
    options.addOption(
        Poco::Util::Option("mhelp", "mh", "Display help about PHYSX Subsystem")
        .required(false)
        .repeatable(false));
}

const char* physx_subsystem::name() const
{
    return "PHYSX-Subsystem";
}


void    physx_subsystem::submitTask (PxBaseTask &task)
{}

uint32_t physx_subsystem::getWorkerCount ()  const
{
    return (32);
}
