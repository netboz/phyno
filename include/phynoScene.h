#pragma once

#include "PxPhysicsAPI.h"

#include "phynoProcessEngine.h"
#include "phynoMqttEvent.h"

#include <stdint.h>

#include <tbb/tbb.h>
#include <tbb/flow_graph.h>
#include <tbb/concurrent_queue.h>

#include "Poco/Timestamp.h"

typedef oneapi::tbb::flow::function_node<oneapi::tbb::flow::continue_msg,
                                         oneapi::tbb::flow::continue_msg,
                                         oneapi::tbb::flow::lightweight>
                                            phynoPhysxPipelineStage_t;


class phynoScene
{
public:
    void run();
    void stop();
    oneapi::tbb::concurrent_queue<phynoEvent *> sceneEventQueue;

    phynoScene(std::string nameIn, physx::PxScene *parentSceneIn) : name(nameIn), parentScene(parentSceneIn)
    {
        eventProcessingStage = new phynoPhysxPipelineStage_t(graph, oneapi::tbb::flow::serial,
                                                             [this](oneapi::tbb::flow::continue_msg) -> oneapi::tbb::flow::continue_msg
                                                             {
                                                                 typedef oneapi::tbb::concurrent_queue<phynoEvent *>::const_iterator iter;
                                                                 for (iter i(this->sceneEventQueue.unsafe_begin()); i != this->sceneEventQueue.unsafe_end(); ++i)
                                                                 {
                                                                    
                                                                 }
                                                                 return (oneapi::tbb::flow::continue_msg());
                                                             });
        SimulationStage = new phynoPhysxPipelineStage_t(graph, oneapi::tbb::flow::serial,
                                                        [this](oneapi::tbb::flow::continue_msg) -> oneapi::tbb::flow::continue_msg
                                                        {
                                                            this->parentScene->simulate(1.0f / 60.0f);
                                                            return (oneapi::tbb::flow::continue_msg());
                                                        });

        renderingStage = new phynoPhysxPipelineStage_t(graph, oneapi::tbb::flow::serial,
                                                       [this](oneapi::tbb::flow::continue_msg) -> oneapi::tbb::flow::continue_msg
                                                       {
                                                           this->parentScene->fetchResults();
                                                           return (oneapi::tbb::flow::continue_msg());
                                                       });

        oneapi::tbb::flow::make_edge(*eventProcessingStage, *SimulationStage);
        oneapi::tbb::flow::make_edge(*SimulationStage, *renderingStage);
    }

    void step(uint16_t stepSize);

    std::string name;
    physx::PxScene *parentScene;

    oneapi::tbb::flow::graph graph;
    Poco::Timestamp lastSimulatedAt;
    bool running;

    phynoPhysxPipelineStage_t *eventProcessingStage;
    phynoPhysxPipelineStage_t *SimulationStage;
    phynoPhysxPipelineStage_t *renderingStage;
};
