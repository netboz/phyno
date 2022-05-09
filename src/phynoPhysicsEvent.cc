#include "phynoPhysicsEvent.h"
#include "Poco/Logger.h"

using namespace Poco;

void phynoEventCreateScene::execute()
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("Adding scene");
}

void phynoEventAddDynamicRigid::execute()
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("Adding actor");
    targetScene->addActor(*rigidDynamics);
}