#include "phynoMqttSubsystem.h"

#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <memory>
#include <sstream>

#include <Poco/Process.h>

#include <mqtt_protocol.h>

void onMosquittoLogCallback(struct mosquitto*, void * /*userData*/, int /* level */, const char *str)
{
    /* Pring all log messages regardless of level. */
    Logger &logger = Logger::get("PhynoMainLogger");
    logger.information("Mosquitto: %?s", std::string(str));
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
        std::stringstream ss;
        ss << sub->processor->phynoPrefix << "/#";
        std::string topic = ss.str();

        logger.information("Subscribing to %?s", topic.c_str());

        mosquitto_subscribe_v5(sub->mosq, NULL, topic.c_str(), QOS, MQTT_SUB_OPT_NO_LOCAL, NULL);
    }
    else
    {
        sub->connected = false;

        logger.warning("Connection error :  %s", mosquitto_reason_string(rc));

        logger.error("Fatal error, stopping phyno.");

        mosquitto_disconnect_v5(sub->mosq, 0, NULL);
        Poco::Process::requestTermination(Poco::Process::id());
    }
}
// void connlost(void *context, char *cause)
// {
//     mqtt_subsystem *sub = (mqtt_subsystem *)context;
//     MQTTAsync client = sub->client;
//     MQTTAsync_connectOptions conn_opts = sub->conn_opts;
//     int rc;
//     sub->self_app->logger().error("MQTT Connection lost. Cause : %s", cause);
//     sub->conn_opts.keepAliveInterval = 20;
//     sub->conn_opts.cleansession = sub->clean_session;
//     if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
//     {
//         sub->self_app->logger().error("Failed to start connection. Return code: %d", rc);
//     }
//     sub->self_app->logger().information("Reconnecting ...");
// }

// int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
// {
//     mqtt_subsystem *sub = (mqtt_subsystem *)context;
//     auto event = new mqttEvent();
//     event->topic = std::string(topicName, topicLen);
//     event->message = message;
//     sub->self_app->logger().information("Message arrived on topic :" + event->topic);
//     // event->message = message;

//     sub->processor->processMqttEvent(event);

//     // inputEventQueueNode.try_put(std::move(event));
//     // MQTTAsync_freeMessage(&message);
//     MQTTAsync_free(topicName);
//     return 1;
// }
// void onDisconnect(void *context, MQTTAsync_successData *response)
// {
//     mqtt_subsystem *sub = (mqtt_subsystem *)context;

//     sub->self_app->logger().information("Successful disconnection.");
//     sub->connected = false;
// }
// void onSubscribe(void *context, MQTTAsync_successData *response)
// {
//     mqtt_subsystem *sub = (mqtt_subsystem *)context;

//     sub->self_app->logger().information("Subscribe succeeded");
// }
// void onSubscribeFailure(void *context, MQTTAsync_failureData *response)
// {
//     mqtt_subsystem *sub = (mqtt_subsystem *)context;

//     sub->self_app->logger().information("Subscribe failed, rc %d", response ? response->code : 0);
// }

// static void onConnectFailure(void *context, MQTTAsync_failureData *response)
// {
//     mqtt_subsystem *sub = (mqtt_subsystem *)context;

//     sub->self_app->logger().information("Connect failed, rc %d", response ? response->code : 0);
//     sub->connected = false;
// }

// static void onConnect(void *context, MQTTAsync_successData *response)
// {
//     mqtt_subsystem *sub = (mqtt_subsystem *)context;
//     sub->connected = true;
//     MQTTAsync client = sub->client;
//     MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
//     sub->self_app->logger().information("Successful connection");

//     if (sub->clean_session)
//     {
//         int rc;
//         std::stringstream ss;
//         ss << sub->processor->phynoPrefix << "/#";
//         std::string topic = ss.str();
//         opts.onSuccess = onSubscribe;
//         opts.onFailure = onSubscribeFailure;
//         opts.subscribeOptions.noLocal = 1;
//         opts.context = context;
//         deliveredtoken = 0;
//         if ((rc = MQTTAsync_subscribe(client, topic.c_str(), QOS, &opts)) != MQTTASYNC_SUCCESS)
//         {
//             sub->self_app->logger().error("Failed to start subscribe, return code %d", rc);
//         }
//     }
//     sub->self_app->logger().information("Initialized MQTT-Subsystem");
// }

mqtt_subsystem::mqtt_subsystem(void) : mqttPrefix(DEFAULT_PHYNO_ROOT_TOPIC)
{

}

mqtt_subsystem::~mqtt_subsystem(void)
{
    // Free mqtt messages processing pipeline ( in case it un initialize wasn't called)
    if (processor)
        delete (processor);
}

void mqtt_subsystem::initialize(Poco::Util::Application &app)
{
    self_app = &app;
    app.logger().information("Initializing MQTT-Subsystem");

    this->processor = new mqttPreProcessor(mqttPrefix);
    // Read configuration values
    std::string broker_url = app.config().getString("phyno.broker.url", DEFAULT_ADDRESS);

    const char *host = "localhost";
    int port = 1883;
    int keepalive = 60;
    bool clean_session = true;

    if (mosquitto_lib_init() != MOSQ_ERR_SUCCESS)
    {
        app.logger().error("FATAL: Failled initialising Mosquitto lib. Terminating application.");
        // Terminating application
        Poco::Process::requestTermination(Poco::Process::id());
    }
    mosq = mosquitto_new("phyno_client", clean_session, this);
    if (!mosq)
    {
        app.logger().error("FATAL: Failled allocating Mosquitto client. Terminating application.");
        // Terminating application
        Poco::Process::requestTermination(Poco::Process::id());
    }

    mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

    mosquitto_connect_v5_callback_set(mosq, onConnectCallback);
    mosquitto_message_v5_callback_set(mosq, onMqttMessageCallback);

    mosquitto_log_callback_set(mosq, onMosquittoLogCallback);

    app.logger().information("Connecting to broker ...");

    if (mosquitto_connect_bind_v5(mosq, host, port, keepalive, NULL, NULL) != MOSQ_ERR_SUCCESS)
    {
        app.logger().error("FATAL: Failled connecting to broker. Terminating application.");
        // Terminating application
        Poco::Process::requestTermination(Poco::Process::id());
    }
    int loop = mosquitto_loop_start(mosq);
    if (loop != MOSQ_ERR_SUCCESS)
    {
        app.logger().error("FATAL: Unable to start network loop. Terminating application.");
        // Terminating application
        Poco::Process::requestTermination(Poco::Process::id());
    }
}

void mqtt_subsystem::reinitialize(Poco::Util::Application &app)
{
    app.logger().information("Re-initializing MQTT-Subsystem");
    uninitialize();
    initialize(app);
}

void mqtt_subsystem::uninitialize()
{
    self_app->logger().information("UN-Initializing MQTT-Subsystem");
    self_app->logger().information("Disconnecting from broker");

    if (mosquitto_disconnect(mosq) != MOSQ_ERR_SUCCESS)
    {
        self_app->logger().information("Failled Disconnecting. Forcing network loop to stop");
        if (mosquitto_loop_stop(mosq, true) != MOSQ_ERR_SUCCESS)
        {
            self_app->logger().information("Failled stopping network thread, terminating application.");

            // Terminating application
            Poco::Process::requestTermination(Poco::Process::id());
        }
    }
    else
    {
        self_app->logger().information("Disconnected from broker.");
        self_app->logger().information("Stopping network loop.");

        if (mosquitto_loop_stop(mosq, false) != MOSQ_ERR_SUCCESS)
        {
            self_app->logger().warning("Failled stopping network thread. Forcing thread termination.");
            if (mosquitto_loop_stop(mosq, true) != MOSQ_ERR_SUCCESS)
            {
                self_app->logger().information("Failled stopping network thread, terminating application.");

                // Terminating application
                Poco::Process::requestTermination(Poco::Process::id());
            }
        }
    }

    mosquitto_lib_cleanup();

    // Delete mqtt message processing pipeline
    if (processor)
        delete (processor);

    self_app->logger().information("UN-Initialized MQTT-Subsystem");
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