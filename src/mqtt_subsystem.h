#pragma once

#include "Poco/Util/Application.h"
#include "Poco/Util/Subsystem.h"
#include "Poco/Logger.h"

#include "MQTTAsync.h"
#include "phynoProcessEngine.h"

using Poco::Util::Subsystem;

#define DEFAULT_ADDRESS       "tcp://localhost:1883"
#define CLIENTID              "phyno"
#define DEFAULT_PHYNO_ROOT_TOPIC      "/physics/phyno"
#define QOS                   1
#define TIMEOUT               10000L

class mqtt_subsystem: public Subsystem
{
public:
	mqtt_subsystem(void);
	~mqtt_subsystem(void);
	
	// Pointer to the application this subsystem belongs to
	Poco::Util::Application *self_app;
    
    //MQTT connection related
    MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	MQTTAsync_token token;
	bool connection_failled = 0;
	bool connected = 0;
	bool clean_session = true;
	mqttPreProcessor *processor;


protected:
	void initialize(Poco::Util::Application &app);
	void reinitialize(Poco::Util::Application &app);
	void uninitialize();
	void defineOptions(Poco::Util::OptionSet& options);

	virtual const char* name() const;

private:
	int rc;
	char p_cName;


};
