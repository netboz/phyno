#include "mqtt_subsystem.h"
#include "phynoProcessEngine.h"
#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <memory>
#include <sstream>

volatile MQTTAsync_token deliveredtoken;

void connlost(void *context, char *cause)
{
    mqtt_subsystem *sub = (mqtt_subsystem *)context;
    MQTTAsync client = sub->client;
    MQTTAsync_connectOptions conn_opts = sub->conn_opts;
    int rc;
    sub->self_app->logger().error("MQTT Connection lost. Cause : %s", cause);
    sub->conn_opts.keepAliveInterval = 20;
    sub->conn_opts.cleansession = sub->clean_session;
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
        sub->self_app->logger().error("Failed to start connection. Return code: %d", rc);
    }
    sub->self_app->logger().information("Reconnecting ...");
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    mqtt_subsystem *sub = (mqtt_subsystem *)context;
    auto event = new mqttEvent();
    event->topic = std::string(topicName, topicLen);
    event->message = message;
    sub->self_app->logger().information("Message arrived on topic :" + event->topic);
    //event->message = message;
    
    
    sub->processor->processMqttEvent(event);

    //inputEventQueueNode.try_put(std::move(event));
    //MQTTAsync_freeMessage(&message); 
    MQTTAsync_free(topicName);
    return 1;
}
void onDisconnect(void* context, MQTTAsync_successData* response)
{
    mqtt_subsystem *sub = (mqtt_subsystem *)context;

    sub->self_app->logger().information("Successful disconnection.");
    sub->connected = false;
}
void onSubscribe(void* context, MQTTAsync_successData* response)
{
    mqtt_subsystem *sub = (mqtt_subsystem *)context;

    sub->self_app->logger().information("Subscribe succeeded");
}
void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
    mqtt_subsystem *sub = (mqtt_subsystem *)context;

    sub->self_app->logger().information("Subscribe failed, rc %d", response ? response->code : 0);
}

static void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
    mqtt_subsystem *sub = (mqtt_subsystem *)context;

    sub->self_app->logger().information("Connect failed, rc %d", response ? response->code : 0);
    sub->connected = false;
}

static void onConnect(void* context, MQTTAsync_successData* response)
{
    mqtt_subsystem *sub = (mqtt_subsystem *)context;
    sub->connected = true;
    MQTTAsync client = sub->client;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    sub->self_app->logger().information("Successful connection");

    if (sub->clean_session)
    {
        int rc;
        std::stringstream ss;
        ss << sub->processor->phynoPrefix << "/#";
        std::string topic = ss.str();
        opts.onSuccess = onSubscribe;
        opts.onFailure = onSubscribeFailure;
        opts.subscribeOptions.noLocal = 1;
        opts.context = context;
        deliveredtoken = 0;
        if ((rc = MQTTAsync_subscribe(client, topic.c_str(), QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
            sub->self_app->logger().error("Failed to start subscribe, return code %d", rc);
        }
    }
    sub->self_app->logger().information("Initialized MQTT-Subsystem");

}

mqtt_subsystem::mqtt_subsystem(void):mqttPrefix(DEFAULT_PHYNO_ROOT_TOPIC) 
{
}

mqtt_subsystem::~mqtt_subsystem(void)
{
    // Free mqtt messages processing pipeline ( in case it un initialize wasn't called)
    if (processor) delete(processor);
}

void mqtt_subsystem::initialize(Poco::Util::Application &app)
{
    self_app = &app;
    app.logger().information("Initializing MQTT-Subsystem");

    this->processor = new mqttPreProcessor(mqttPrefix);
    //Read configuration values
    std::string broker_url = app.config().getString("phyno.broker.url", DEFAULT_ADDRESS);

    int rc;
    MQTTAsync_create(&client, broker_url.data(), CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_setCallbacks(client, this, connlost, msgarrvd, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = clean_session;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = this;
    
    app.logger().information("Connecting to broker ...");
    
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
        app.logger().error("Failed to start connect, return code %d", rc);
        //  exit(EXIT_FAILURE);
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
    int rc;
    self_app->logger().information("UN-Initializing MQTT-Subsystem");
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    //disc_opts.onSuccess = onDisconnect;
    if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
    {
        self_app->logger().error("Failed to start disconnect, return code %d", rc);
    }
    
    // Delete mqtt message processing pipeline
    if (processor) delete(processor);

    MQTTAsync_destroy(&client);
    self_app->logger().information("UN-Initialized MQTT-Subsystem");

}

void mqtt_subsystem::defineOptions(Poco::Util::OptionSet& options)
{
    Subsystem::defineOptions(options);
    options.addOption(
        Poco::Util::Option("mhelp", "mh", "Display help about MQTT Subsystem")
        .required(false)
        .repeatable(false));
}

const char* mqtt_subsystem::name() const
{
    return "MQTT-Subsystem";
}