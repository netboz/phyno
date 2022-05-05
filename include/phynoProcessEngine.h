#pragma once

#include <tbb/tbb.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/flow_graph.h>

#include "MQTTAsync.h"

#include "Poco/StringTokenizer.h"
#include "Poco/Logger.h"

#include <tuple>

#include "physx_subsystem.h"
#include "phynoEvent.h"
#include "mqttEvent.h"

using Poco::Logger;
using Poco::StringTokenizer;
using namespace physx;

struct Edge
{
	int src, dest;
	std::string weight;
};

typedef std::pair<int, std::string> Pair;

// A class to represent a graph object
class Graph
{
public:
	// a vector of vectors of Pairs to represent an adjacency list
	std::vector<std::vector<Pair>> adjList;

	// Graph Constructor
	Graph(std::vector<Edge> const &edges, int n)
	{
		// resize the vector to hold `n` elements of type vector<Edge>
		adjList.resize(n);

		// add edges to the directed graph
		for (auto &edge : edges)
		{
			int src = edge.src;
			int dest = edge.dest;
			std::string weight = edge.weight;

			// insert at the end
			adjList[src].push_back(make_pair(dest, weight));

			// uncomment the following code for undirected graph
			// adjList[dest].push_back(make_pair(src, weight));
		}
	}
};

// total number of nodes in the graph (labelled from 0 to 5)
static int graphSize = 4;

static std::vector<Edge> edges =
	{
		// (x, y, w) —> edge from `x` to `y` having weight `w`
		{0, 1, "scenes"},
		{1, 2, "$scene_name"},
		{2, 3, "entities"},
		{3, 4, "$entity_name"}};

static Graph graph(edges, graphSize);

typedef std::tuple<mqttEvent *> mqttEventTaskSelectorTupleIn_t;
typedef std::tuple<mqttEvent *, mqttEvent *, mqttEvent *, mqttEvent *, mqttEvent *> mqttEventTaskSelectorTupleOut_t;

typedef oneapi::tbb::flow::function_node<mqttEvent *, phynoEvent *> taskMqttEventFunctionNode_t;
typedef oneapi::tbb::flow::multifunction_node<mqttEventTaskSelectorTupleIn_t,
											  mqttEventTaskSelectorTupleOut_t, oneapi::tbb::flow::lightweight>
	taskMqttEventMultiFunctionNode_t;

typedef phynoEvent *(*mqttEventTaskFunc_t)(mqttEvent *);

class mqttEventTaskSelectorBody
{
public:
	mqttEventTaskSelectorBody(const std::string &phynoPrefixIn) : phynoPrefix(phynoPrefixIn) {}

	std::string phynoPrefix;
	std::string removePrefix(const std::string &str)
	{
		std::string result1;
		if (isPrefix(phynoPrefix, str))
			return (str.substr(phynoPrefix.length()));
		else
			return (str);
	}

	// TODO: Turn this into macro
	bool isPrefix(const std::string &prefix, const std::string &target)
	{

		auto res = std::mismatch(prefix.begin(), prefix.end(), target.begin());

		if (res.first == prefix.end())
		{
			return (true);
		}
		return (false);
	}

	uint8_t findNextNode(uint8_t enteringNode, StringTokenizer::Iterator index, StringTokenizer::Iterator end, std::map<std::string, std::string> &paramParsed)
	{
		Poco::Logger *logger;
		logger = &Logger::get("PhynoMainLogger");

		if (index == end)
		{
			logger->information("End of URL");
			return enteringNode;
		}

		std::vector<Pair>::iterator itBegin = graph.adjList[enteringNode].begin();
		std::vector<Pair>::iterator itEnd = graph.adjList[enteringNode].end();
		logger->information("Entering node %?i", enteringNode);
		std::vector<Pair>::iterator it = std::find_if(
			itBegin,
			itEnd,
			[index, &paramParsed](Pair p)
			{
												Poco::Logger *logger;
												logger = &Logger::get("PhynoMainLogger");
												logger->information("string url %?s   string graph %?s", *index, p.second);
												if (p.second[0] == '$'){
													logger->information("Variable ");
													paramParsed.insert ( std::pair<std::string, std::string>(p.second.substr(1),*index) );
													logger->information("key : %?s",p.second.substr(1));
													return(true);
													}

												return(p.second == *index); });

		if (it != itEnd)
		{
			return (findNextNode((*it).first, ++index, end, paramParsed));
		}
		else
			return (enteringNode);
	}

	void operator()(const mqttEventTaskSelectorTupleIn_t &in, taskMqttEventMultiFunctionNode_t::output_ports_type &ports)
	{
		Poco::Logger *logger;

		logger = &Logger::get("PhynoMainLogger");
		logger->information("Multifuncnode");
		mqttEvent *event = std::get<0>(in);
		std::string phynoTopic = removePrefix(event->topic);

		StringTokenizer tokenizer(phynoTopic, "/", StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);
		StringTokenizer::Iterator index = tokenizer.begin();
		StringTokenizer::Iterator end = tokenizer.end();
		uint8_t currentNode = 0;
		logger->information("Resolving node");

		uint8_t nextNode = findNextNode(currentNode, index, end, event->paramParsed);
		logger->information("Resolved node %?i", nextNode);
		switch (nextNode)
		{
		case 0:
			std::get<0>(ports).try_put(event);
			break;

		case 1:
			logger->information("No Handler for scene topic");
			break;

		case 2:
			std::get<2>(ports).try_put(event);
			break;

		case 3:
			logger->information("No Handler for entity topic");
			break;

		case 4:
			std::get<4>(ports).try_put(event);
			break;
		}
	}
};

// Create multifunction node
class mqttPreProcessor
{
public:
	mqttPreProcessor(const std::string &str);
	~mqttPreProcessor();
	std::string removePrefix(const std::string &str);

	std::string phynoPrefix;

	std::vector<taskMqttEventFunctionNode_t *> vectorTasks;

	// preprocessingMqttEventTask_t createSceneTask;
	void processMqttEvent(mqttEvent *mqttEvent);
	oneapi::tbb::flow::graph graph;

	taskMqttEventMultiFunctionNode_t *mqttEventTaskSelector;

private:
	Poco::Logger *logger;
};
