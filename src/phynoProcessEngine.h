#pragma once

#include <tbb/tbb.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/flow_graph.h>

#include "MQTTAsync.h"

#include "Poco/StringTokenizer.h"
#include "Poco/Logger.h"

#include <tuple>

using Poco::Logger;
using Poco::StringTokenizer;

static std::vector<std::vector<std::string>> parsingGraphAdjencyTable
{
	{"", 	"scenes", 	"",			"",				"", 	""},
	{"",	"",			"$1",		"",				"", 	""},
	{"",	"",			"",			"entities",		"", 	""},
	{"",	"",			"",			"",				"$2", 	""}

};


class mqttEvent
{
public:
	std::string topic;
	MQTTAsync_message *message;
};


class phynoEvent
{
	std::chrono::time_point<std::chrono::system_clock> date;
};
typedef std::tuple<mqttEvent *> mqttEventTaskSelectorTupleIn_t;
typedef std::tuple<mqttEvent *, mqttEvent *> mqttEventTaskSelectorTupleOut_t;

typedef oneapi::tbb::flow::function_node<mqttEvent*, phynoEvent*> taskMqttEventFunctionNode_t;
typedef oneapi::tbb::flow::multifunction_node<mqttEventTaskSelectorTupleIn_t,
        mqttEventTaskSelectorTupleOut_t, oneapi::tbb::flow::lightweight> taskMqttEventMultiFunctionNode_t;

typedef phynoEvent *(*mqttEventTaskFunc_t)(mqttEvent *);


class mqttEventTaskSelectorBody
{
public:
	mqttEventTaskSelectorBody(const std::string& phynoPrefixIn): phynoPrefix(phynoPrefixIn) {}

	std::string phynoPrefix;
	std::string removePrefix(const std::string& str)
	{
		std::string result1 ;
		if (isPrefix(phynoPrefix, str))
			return (str.substr(phynoPrefix.length()));
		else
			return (str);
	}

//TODO: Turn this into macro
	bool isPrefix(const std::string &prefix, const std::string &target)
	{

		auto res = std::mismatch(prefix.begin(), prefix.end(), target.begin());

		if (res.first == prefix.end())
		{
			return (true);
		}
		return (false);
	}
	uint8_t findNextNode(uint8_t enteringNode, StringTokenizer::Iterator index, StringTokenizer::Iterator end)
	{
		if (index == end) return enteringNode;

		std::vector<std::string>::iterator itBegin = parsingGraphAdjencyTable[enteringNode].begin();
		std::vector<std::string>::iterator itEnd = parsingGraphAdjencyTable[enteringNode].end();

		std::vector<std::string>::iterator it = std::find(itBegin, itEnd, *index);

		if (it != itEnd)
		{
			return (findNextNode(it - itBegin, ++index, end));
		}
		else
			return (enteringNode);
	}

	void operator()(const mqttEventTaskSelectorTupleIn_t &in , taskMqttEventMultiFunctionNode_t::output_ports_type &ports) {
		Poco::Logger *logger;

		map<std::string, std::string> paramParsed;

		logger = &Logger::get("PhynoMainLogger");
		logger->information("Multifuncnode");
		mqttEvent *event = std::get<0>(in);
		std::string phynoTopic = removePrefix(event->topic);

		StringTokenizer tokenizer(phynoTopic, "/", StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);
		StringTokenizer::Iterator index = tokenizer.begin();
		StringTokenizer::Iterator end = tokenizer.end();
		uint8_t currentNode = 0;
		logger->information("Resolving node");

		uint8_t nextNode = findNextNode(currentNode, index, end);
		logger->information("Resolved node %?i", nextNode);
		switch  (nextNode )
		{
		case 0:
			std::get<0>(ports).try_put(event);
			break;

		case 1:
			std::get<1>(ports).try_put(event);
			break;
		}

	}
};

// Create multifunction node
class mqttPreProcessor
{
public:
	mqttPreProcessor(const std::string& str);
	~mqttPreProcessor();

	std::string removePrefix(const std::string& str);

	std::string phynoPrefix;

	std::vector<taskMqttEventFunctionNode_t *> vectorTasks;

	//preprocessingMqttEventTask_t createSceneTask;
	void processMqttEvent(mqttEvent* mqttEvent);
	oneapi::tbb::flow::graph graph;

	taskMqttEventMultiFunctionNode_t *mqttEventTaskSelector;

private:
	Poco::Logger *logger;

};


