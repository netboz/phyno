#include "phynoScene.h"
#include <stdint.h>

void phynoScene::run()
{
    running = true;
    eventProcessingStage->try_put(oneapi::tbb::flow::continue_msg());
}

void phynoScene::step(uint16_t /*stepSize*/)
{
    eventProcessingStage->try_put(oneapi::tbb::flow::continue_msg());
}

void phynoScene::stop()
{
    running = false;
}

phynoScene::~phynoScene()
{
    delete (eventProcessingStage);
    delete (SimulationStage);
    delete (renderingStage);
}