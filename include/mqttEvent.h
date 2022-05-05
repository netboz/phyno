#pragma once

class mqttEvent
{
public:
	std::string topic;
	MQTTAsync_message *message;
	std::map<std::string, std::string> paramParsed;
	~mqttEvent() { MQTTAsync_freeMessage(&message); };
};
