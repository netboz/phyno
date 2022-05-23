#include "Poco/Logger.h"
#include "Poco/Util/Application.h"

#include "phynoPhysxCallbacks.h"
#include "phynoMqttSubsystem.h"
#include "phynoScene.h"

#include <iostream>

#include "yyjson.h"

using Poco::Logger;

void ephysx_simulation_callbacks::onConstraintBreak(PxConstraintInfo * /*constraints*/, PxU32 /*count*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("PHYSX_CALLBACKS: Constraint break");
}

void ephysx_simulation_callbacks::onWake(PxActor ** /*actors*/, PxU32 /*count*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("PHYSX_CALLBACKS: onWake");
}

void ephysx_simulation_callbacks::onSleep(PxActor ** /*actors*/, PxU32 /*count*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("PHYSX_CALLBACKS: onSleep");
}

void ephysx_simulation_callbacks::onContact(const PxContactPairHeader & /*pairHeader*/, const PxContactPair * /*pairs*/, PxU32 /*nbPairs*/)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("PHYSX_CALLBACKS: onContact");
}

void ephysx_simulation_callbacks::onTrigger(PxTriggerPair * /*pairs*/, PxU32 /*count*/)
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
        yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);

        yyjson_mut_val *rootObj = yyjson_mut_obj(doc);

        // Genereta json for location
        const char *jPosKeys[] = {"x", "y", "z"};
        const char *jPosVals[] = {std::to_string(trans.p.x).c_str(), std::to_string(trans.p.y).c_str(), std::to_string(trans.p.z).c_str()};

        yyjson_mut_val *jPos = yyjson_mut_obj_with_str(doc, jPosKeys, jPosVals, 3);
        yyjson_mut_val *jpkey = yyjson_mut_str(doc, "l");
        yyjson_mut_obj_add(rootObj, jpkey, jPos);

        // Genereta json for orientation
        const char *jOriKeys[] = {"x", "y", "z", "z"};
        const char *jOriVals[] = {std::to_string(trans.q.x).c_str(), std::to_string(trans.q.y).c_str(),
                                  std::to_string(trans.q.z).c_str(), std::to_string(trans.q.w).c_str()};

        yyjson_mut_val *jOri = yyjson_mut_obj_with_str(doc, jOriKeys, jOriVals, 3);
        yyjson_mut_val *jorikey = yyjson_mut_str(doc, "o");

        yyjson_mut_obj_add(rootObj, jorikey, jOri);

        // Genereta json for linear velocity
        PxVec3 Vel = bodyBuffer[i]->getLinearVelocity();

        const char *jLVelKeys[] = {"x", "y", "z"};
        const char *jLVelVals[] = {std::to_string(Vel.x).c_str(), std::to_string(Vel.y).c_str(), std::to_string(Vel.z).c_str()};

        yyjson_mut_val *jLVel = yyjson_mut_obj_with_str(doc, jLVelKeys, jLVelVals, 3);
        yyjson_mut_val *jlvelkey = yyjson_mut_str(doc, "lv");

        yyjson_mut_obj_add(rootObj, jlvelkey, jLVel);

        // Genereta json for angular velocity
        PxVec3 aVel = bodyBuffer[i]->getAngularVelocity();

        const char *jAVelKeys[] = {"x", "y", "z"};
        const char *jAVelVals[] = {std::to_string(aVel.x).c_str(), std::to_string(aVel.y).c_str(), std::to_string(aVel.z).c_str()};

        yyjson_mut_val *jAVel = yyjson_mut_obj_with_str(doc, jAVelKeys, jAVelVals, 3);
        yyjson_mut_val *javelkey = yyjson_mut_str(doc, "ov");

        yyjson_mut_obj_add(rootObj, javelkey, jAVel);

        yyjson_mut_doc_set_root(doc, rootObj);
        size_t len;

        char *json = yyjson_mut_write(doc, YYJSON_WRITE_ESCAPE_UNICODE, &len);

        std::string topic = "/scenes/" + pScene->name + "/actors/" + agentName + "/pos/";

        mqttSub.send(topic, json, len, true);
    }
}
