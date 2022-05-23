#pragma once
#include "mosquitto.h"


class mqttEvent
{
public:
	std::string topic;
	std::string payload;
	std::map<std::string, std::string> paramParsed;
	
	~mqttEvent() { };
};
