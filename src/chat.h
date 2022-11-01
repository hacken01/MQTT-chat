#ifndef MQTT_CHAT
#define MQTT_CHAT

#include "display.h"
#include "frozen.h"
#include "log.h"
#include <MQTTClient.h>
#include <ctype.h> // for the isdigit function
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define UNUSED(x) (void)(x)

#define MQTT_KEEP_ALIVE_INTERVAL 30
#define MQTT_CLEAN_SESSION 0
#define MQTT_SUB_QOS 1
#define MQTT_PUB_QOS 1
#define STATUS_BUF_SIZE 1024
#define CHAT_MAX_SIZE 1024
#define CLIENT_ONLINE 1
#define CLIENT_OFFLINE 0
#define CHAT_MESSAGE_TOPIC "+/message"
#define CHAT_STATUS_TOPIC "+/status"
#define CHAT_PUBLISH_STATUS "status"
#define CHAT_PUBLISH_MESSAGE "message"
#define CHAT_DEFAULT_PORT "1883"
#define CHAT_DEFAULT_HOST "localhost"

typedef struct Config {
    char *port;
    char *host;
    char *netid;
    char *name;
} Config;

#define BAD_OPTION -1

// Global variables
volatile MQTTClient_deliveryToken deliveredtoken;

// prototype functions
void errorMessage();
void helpMessage();
bool checkPort(char *port);
void parse_arguments(Config *config, int argc, char *argv[]);
void received_input(char *data);
void publish_message(char *jsonMessage, bool retained);
void onlineStatus();
void offlineStatus();
void close_connection();
void chat_init(Config *config);

#endif
