#pragma once

#include "Poco/Util/Application.h"
#include "Poco/Util/Subsystem.h"
#include "Poco/Logger.h"

#include <mosquitto.h>

#include "phynoProcessEngine.h"

using Poco::Util::Subsystem;

#define DEFAULT_ADDRESS "localhost"
#define DEFAULT_PORT 1883

#define CLIENTID "phyno"
#define DEFAULT_PHYNO_ROOT_TOPIC "/physics/phyno"
#define QOS 0
#define TIMEOUT 10000L

class mqtt_subsystem : public Subsystem
{
public:
	mqtt_subsystem(void);
	~mqtt_subsystem(void);

	void send(std::string topic, const char *payload, uint32_t length)
	{
		mosquitto_publish_v5(this->mosq, NULL, (mqttPrefix + topic).c_str(), length, payload, QOS, false, NULL);
	}

	void send(std::string topic, const char *payload, uint32_t length, bool retain)
	{
		mosquitto_publish_v5(this->mosq, NULL, (mqttPrefix + topic).c_str(), length, payload, QOS, retain, NULL);
	}
	// Pointer to the application this subsystem belongs to
	Poco::Util::Application *self_app;

	// MQTT prefix to all topics
	std::string mqttPrefix;

	// MQTT connection related
	struct mosquitto *mosq;

	bool connection_failled = 0;
	bool connected = 0;
	bool clean_session = true;
	mqttPreProcessor *processor;

protected:
	void initialize(Poco::Util::Application &app);
	void reinitialize(Poco::Util::Application &app);
	void uninitialize();
	void defineOptions(Poco::Util::OptionSet &options);

	virtual const char *name() const;

private:
	int rc;
	char p_cName;
};
