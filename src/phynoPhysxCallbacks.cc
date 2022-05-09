#include "Poco/Logger.h"
#include "Poco/Util/Application.h"

#include "phynoPhysxCallbacks.h"
#include "phynoMqttSubsystem.h"
#include "phynoScene.h"

#include <iostream>

using Poco::Logger;

void ephysx_simulation_callbacks::onConstraintBreak(PxConstraintInfo */*constraints*/, PxU32 /*count*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("PHYSX_CALLBACKS: Constraint break");
}

void ephysx_simulation_callbacks::onWake(PxActor **/*actors*/, PxU32 /*count*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("PHYSX_CALLBACKS: onWake");
}

void ephysx_simulation_callbacks::onSleep(PxActor **/*actors*/, PxU32 /*count*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("PHYSX_CALLBACKS: onSleep");
}

void ephysx_simulation_callbacks::onContact(const PxContactPairHeader &/*pairHeader*/, const PxContactPair */*pairs*/, PxU32 /*nbPairs*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("PHYSX_CALLBACKS: onContact");
}

void ephysx_simulation_callbacks::onTrigger(PxTriggerPair */*pairs*/, PxU32 /*count*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("PHYSX_CALLBACKS: onTrigger");
}

void ephysx_simulation_callbacks::onAdvance(const PxRigidBody *const *bodyBuffer, const PxTransform *poseBuffer, const PxU32 count)
{
    mqtt_subsystem &mqttSub = Poco::Util::Application::instance().getSubsystem<mqtt_subsystem>();
    for (PxU32 i = 0; i < count; i++)
    {
        std::string agentName(*(std::string *)(bodyBuffer[i]->userData));
        PxScene *scene = bodyBuffer[i]->getScene();
        phynoScene *pScene = (phynoScene *)scene->userData;
        PxTransform trans = poseBuffer[i];

        std::string x = std::to_string(trans.p.x);
        std::string y = std::to_string(trans.p.y);
        std::string z = std::to_string(trans.p.z);

        std::string loc(x + ":" + y + ":" + z);
        std::string topic = "/scenes/" + pScene->name + "/entities/" + agentName + "/pos/";
        mqttSub.send((topic + "/loc").c_str(), const_cast<char *>(loc.c_str()), loc.size(), true);

        x = std::to_string(trans.q.x);
        y = std::to_string(trans.q.y);
        z = std::to_string(trans.q.z);
        std::string w = std::to_string(trans.q.w);
        
        std::string orientation(x + ":" + y + ":" + z + ":" + w);

        mqttSub.send((topic + "/ori").c_str(), const_cast<char *>(orientation.c_str()), orientation.size(), true);

        PxVec3 Vel = bodyBuffer[i]->getLinearVelocity();
        x = std::to_string(Vel.x);
        y = std::to_string(Vel.y);
        z = std::to_string(Vel.z);

        std::string lvel(x + ":" + y + ":" + z + ":" + w);

        mqttSub.send((topic + "/lvel").c_str(), const_cast<char *>(lvel.c_str()), lvel.size(), true);

        PxVec3 aVel = bodyBuffer[i]->getAngularVelocity();
        x = std::to_string(aVel.x);
        y = std::to_string(aVel.y);
        z = std::to_string(aVel.z);

        std::string lavel(x + ":" + y + ":" + z + ":" + w);

        mqttSub.send((topic + "/avel").c_str(), const_cast<char *>(lavel.c_str()), lavel.size(), true);

    }
}
