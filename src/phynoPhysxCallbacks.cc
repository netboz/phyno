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
    logger.error("Constraint break");
}

void ephysx_simulation_callbacks::onWake(PxActor **/*actors*/, PxU32 /*count*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.error("onWake");
}

void ephysx_simulation_callbacks::onSleep(PxActor **/*actors*/, PxU32 /*count*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.error("onSleep");
}

void ephysx_simulation_callbacks::onContact(const PxContactPairHeader &/*pairHeader*/, const PxContactPair */*pairs*/, PxU32 /*nbPairs*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.error("onContact");
}

void ephysx_simulation_callbacks::onTrigger(PxTriggerPair */*pairs*/, PxU32 /*count*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.error("onTrigger");
}

void ephysx_simulation_callbacks::onAdvance(const PxRigidBody *const *bodyBuffer, const PxTransform */*poseBuffer*/, const PxU32 count)
{
    mqtt_subsystem &mqttSub = Poco::Util::Application::instance().getSubsystem<mqtt_subsystem>();
    for (PxU32 i = 0; i < count; i++)
    {
        std::string agentName(*(std::string *)(bodyBuffer[i]->userData));
        PxScene *scene = bodyBuffer[i]->getScene();
        phynoScene *pScene = (phynoScene *)scene->userData;
        PxTransform trans = bodyBuffer[i]->getGlobalPose();

        std::string x = std::to_string(trans.p.x);
        std::string y = std::to_string(trans.p.y);
        std::string z = std::to_string(trans.p.z);

        std::string loc(x + ":" + y + ":" + z);
        std::string topic = "/scenes/" + pScene->name + "/entities/" + agentName + "/pos/";
        mqttSub.send((topic + "/loc").c_str(), const_cast<char *>(loc.c_str()), loc.size());

        x = std::to_string(trans.q.x);
        y = std::to_string(trans.q.y);
        z = std::to_string(trans.q.z);
        std::string w = std::to_string(trans.q.w);
        
        std::string orientation(x + ":" + y + ":" + z + ":" + w);

        mqttSub.send((topic + "/ori").c_str(), const_cast<char *>(orientation.c_str()), orientation.size());
    }
}
