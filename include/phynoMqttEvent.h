#pragma once
#include "mosquitto.h"

class mqttEvent
{
public:
	std::string topic;
	std::map<std::string, std::string> paramParsed;
	const struct 	mosquitto_message 	*message;
	~mqttEvent() { };
};
