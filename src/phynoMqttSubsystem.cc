#include "phynoMqttSubsystem.h"

#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <memory>
#include <sstream>

#include <Poco/Process.h>

#include <mqtt_protocol.h>

void onMosquittoLogCallback(struct mosquitto*, void * /*userData*/, int level, const char *str)
{
    /* Pring all log messages regardless of level. */
    if (level == 16) return;
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("MOSQUITTO_LAYER level %d: %?s", level, std::string(str));
}

void onMqttMessageCallback(struct mosquitto *, void *userData, const struct mosquitto_message *message, const mosquitto_property *)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("Message on Topic : %?s  Payload :%?s", std::string((const char *)message->topic), std::string((const char *)message->payload, message->payloadlen));
    auto event = new mqttEvent();
    mqtt_subsystem *sub = (mqtt_subsystem *)userData;

    event->topic = std::string(message->topic);
    event->message = message;
    sub->processor->processMqttEvent(event);
}
void onConnectCallback(struct mosquitto *, void *userData, int rc, int /*flags*/, const mosquitto_property *)
{
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("Connected to broker");
    mqtt_subsystem *sub = (mqtt_subsystem *)userData;

    if (rc == 0)
    {
        sub->connected = true;

        std::string topic = sub->processor->phynoPrefix + "/#";

        logger.information("MQTT_SUBSYSTEM: Subscribing to %?s", topic.c_str());

        mosquitto_subscribe_v5(sub->mosq, NULL, topic.c_str(), QOS, MQTT_SUB_OPT_NO_LOCAL, NULL);
    }
    else
    {
        sub->connected = false;

        logger.warning("MQTT_SUBSYSTEM: Connection error :  %s", mosquitto_reason_string(rc));

        logger.error("MQTT_SUBSYSTEM: Fatal error, stopping phyno.");

        mosquitto_disconnect_v5(sub->mosq, 0, NULL);
        Poco::Process::requestTermination(Poco::Process::id());
    }
}

mqtt_subsystem::mqtt_subsystem(void) : mqttPrefix(DEFAULT_PHYNO_ROOT_TOPIC)
{

}

mqtt_subsystem::~mqtt_subsystem(void)
{
   
}

void mqtt_subsystem::initialize(Poco::Util::Application &app)
{
    self_app = &app;
    app.logger().information("MQTT_SUBSYSTEM: Initializing MQTT-Subsystem");

    this->processor = new mqttPreProcessor(mqttPrefix);
    // Read configuration values
    std::string broker_url = app.config().getString("phyno.broker.url", DEFAULT_ADDRESS);
    int broker_port = app.config().getInt("phyno.broker.url", DEFAULT_PORT);

    int keepalive = 60;
    bool clean_session = true;


    if (mosquitto_lib_init() != MOSQ_ERR_SUCCESS)
    {
        app.logger().error("MQTT_SUBSYSTEM: FATAL: Failled initialising Mosquitto lib. Terminating application.");
        // Terminating application
        Poco::Process::requestTermination(Poco::Process::id());
    }
    mosq = mosquitto_new("phyno_client", clean_session, this);
    if (!mosq)
    {
        app.logger().error("MQTT_SUBSYSTEM: FATAL: Failled allocating Mosquitto client. Terminating application.");
        // Terminating application
        Poco::Process::requestTermination(Poco::Process::id());
    }

    mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

    mosquitto_connect_v5_callback_set(mosq, onConnectCallback);
    mosquitto_message_v5_callback_set(mosq, onMqttMessageCallback);

    mosquitto_log_callback_set(mosq, onMosquittoLogCallback);

    app.logger().information("MQTT_SUBSYSTEM: Connecting to broker ...");

    if (mosquitto_connect_bind_v5(mosq, broker_url.c_str(), broker_port, keepalive, NULL, NULL) != MOSQ_ERR_SUCCESS)
    {
        app.logger().error("MQTT_SUBSYSTEM: FATAL: Failled connecting to broker. Terminating application.");
        // Terminating application
        Poco::Process::requestTermination(Poco::Process::id());
    }
    int loop = mosquitto_loop_start(mosq);
    if (loop != MOSQ_ERR_SUCCESS)
    {
        app.logger().error("MQTT_SUBSYSTEM: FATAL: Unable to start network loop. Terminating application.");
        // Terminating application
        Poco::Process::requestTermination(Poco::Process::id());
    }
}

void mqtt_subsystem::reinitialize(Poco::Util::Application &app)
{
    app.logger().information("MQTT_SUBSYSTEM: Re-initializing MQTT-Subsystem");
    uninitialize();
    initialize(app);
}

void mqtt_subsystem::uninitialize()
{
    self_app->logger().information("MQTT_SUBSYSTEM: UN-Initializing MQTT-Subsystem");
    self_app->logger().information("MQTT_SUBSYSTEM: Disconnecting from broker");

    if (mosquitto_disconnect(mosq) != MOSQ_ERR_SUCCESS)
    {
        self_app->logger().information("MQTT_SUBSYSTEM: Failled Disconnecting. Forcing network loop to stop");
        if (mosquitto_loop_stop(mosq, true) != MOSQ_ERR_SUCCESS)
        {
            self_app->logger().information("MQTT_SUBSYSTEM: Failled stopping network thread, terminating application.");

            // Terminating application
            Poco::Process::requestTermination(Poco::Process::id());
        }
    }
    else
    {
        self_app->logger().information("MQTT_SUBSYSTEM: Disconnected from broker.");

        self_app->logger().information("MQTT_SUBSYSTEM: Stopping network loop.");

        if (mosquitto_loop_stop(mosq, false) != MOSQ_ERR_SUCCESS)
        {
            self_app->logger().warning("MQTT_SUBSYSTEM: Failled stopping network thread. Forcing thread termination.");
            if (mosquitto_loop_stop(mosq, true) != MOSQ_ERR_SUCCESS)
            {
                self_app->logger().information("MQTT_SUBSYSTEM: Failled stopping network thread, terminating application.");

                // Terminating application
                Poco::Process::requestTermination(Poco::Process::id());
            }
        }
    }
    mosquitto_destroy(mosq);

    mosquitto_lib_cleanup();

    // Delete mqtt message processing pipeline
    if (processor)
        delete (processor);

    self_app->logger().information("MQTT_SUBSYSTEM: UN-Initialized MQTT-Subsystem");
}

void mqtt_subsystem::defineOptions(Poco::Util::OptionSet &options)
{
    Subsystem::defineOptions(options);
    options.addOption(
        Poco::Util::Option("mhelp", "mh", "Display help about MQTT Subsystem")
            .required(false)
            .repeatable(false));
}

const char *mqtt_subsystem::name() const
{
    return "MQTT-Subsystem";
}