#include "chat.h"
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient client;
char *name;
char *netid;
/****************************************************************************
 *                  FUNCTIONS CALBACK RELATED
 * *************************************************************************/
// function to check for message delivery
void delivered(void *context, MQTTClient_deliveryToken dt) {
    UNUSED(context);
    log_debug("Message delivered");
    deliveredtoken = dt;
}
// check for message arrived
int messages(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    UNUSED(context);
    UNUSED(topicLen);
    if (message->payload != NULL && topicName != NULL) {
        char *chat_message = calloc(CHAT_MAX_SIZE, sizeof(char *));
        char client_time[CHAT_MAX_SIZE];
        char *name_received = calloc(CHAT_MAX_SIZE, sizeof(char *));
        char *message_only = calloc(CHAT_MAX_SIZE, sizeof(char *));
        int client_status;
        char *pChar;

        pChar = strchr(topicName, '/'); // find the index of the space char
        pChar = pChar + 1;
        log_debug("TOPIC NAME: %s", topicName);
        log_info("Data: %s", message->payload);

        if (strcmp(pChar, "status") == 0) {
            char *format_status = "{ name: %Q, online: %B }";
            // wait for the first message from broker.
            json_scanf(message->payload, message->payloadlen, format_status, &name_received,
                       &client_status);

            if (name_received != NULL) {
                if (client_status != 0) {
                    strcpy(chat_message, name_received);
                    strcat(chat_message, " is online\n");
                    strcat(chat_message, "\0");
                    display_line(chat_message);
                } else {
                    strcpy(chat_message, name_received);
                    strcat(chat_message, " is Offline\n");
                    strcat(chat_message, "\0");
                    display_line(chat_message);
                }
            }
        } else if (strcmp(pChar, "message") == 0) {
            char *format_messages = "{timestamp: %Q, name: %Q, message: %Q }";
            json_scanf(message->payload, message->payloadlen, format_messages, &client_time,
                       &name_received, &message_only);

            if (message_only != NULL) {
                time_t now;
                struct tm ts;
                // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
                ts = *localtime(&now);
                strftime(client_time, sizeof(client_time), "%R %D", &ts);
                strcpy(chat_message, client_time);
                strcat(chat_message, "  ");
                strcat(chat_message, name_received);
                strcat(chat_message, " : ");
                strcat(chat_message, message_only);
                log_info("Message: %s", message_only);
                display_line(chat_message);
            }
        }

        free(name_received);
        free(chat_message);
        free(message_only);
    }

    // MQTTClient_freeMessage(&message); // seg fault if uncommented
    // MQTTClient_free(topicName);       // seg fault if uncommented
    return EXIT_SUCCESS;
}
// check if the connection have been lost
void connlost(void *context, char *cause) {
    UNUSED(context);
    log_debug("CONNECTION LOST");
    fprintf(stderr, "\nConnection lost by %s\n", cause);

    if ((MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to reconnect");
        connlost(context, cause);
    }
}
/****************************************************************************
 *                  FUNCTIONS CHAT realated
 * *************************************************************************/
// get the first option to establish connection
void parse_arguments(Config *config, int argc, char *argv[]) {
    // initialize the variables
    config->port = CHAT_DEFAULT_PORT;
    config->host = CHAT_DEFAULT_HOST;
    config->name = NULL;
    config->netid = NULL;
    name = NULL;
    netid = NULL;
    log_set_quiet(true);

    FILE *log_file = fopen("log.txt", "w");
    log_add_fp(log_file, LOG_TRACE);
    int c = 0;

    // display error message if executed without netid
    if (argc == 1)
        errorMessage();

    while (1) {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        static struct option long_options[] = {
            /* These options set a flag. */
            {"verbose", optional_argument, 0, 'v'}, // any value not zero is true, so v is true
            {"help", optional_argument, 0, 'a'},
            {"host", required_argument, 0, 'h'},
            {"port", required_argument, 0, 'p'},
            {"name", optional_argument, 0, 'n'},
            {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "vah:p:n:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == BAD_OPTION)
            break;
        switch (c) {
        case 'a': // help menu handled
            helpMessage();
            break;
        case 'v':
            log_set_quiet(false);
            log_trace("argc: %d", argc);
            break;
        case 'h':
            config->host = optarg;
            log_trace("HOSTNAME %s", optarg);
            if (optind + 1 > argc) // missing arguments
                errorMessage();
            break;
        case 'p':
            config->port = optarg;
            log_trace("Port: %s", optarg);
            if (checkPort(config->port)) {
                fprintf(stderr, "Invalid Port: Please try another port\n");
                errorMessage();
            }
            if (optind + 1 > argc) // missing arguments
                errorMessage();
            break;
        case 'n':
            name = config->name = optarg;
            if (optind + 1 > argc) // missing arguments
                errorMessage();
            break;
        case '?':
            errorMessage();
            break;
        default:
            exit(EXIT_FAILURE);
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc) {
        log_debug("Remaining Arguments");
        if (argv[optind] != NULL)
            netid = config->netid = argv[optind++]; // increment for the netID
        if (config->netid == NULL) {
            errorMessage();
        }
        if (optind < argc) {
            fprintf(stderr, "Unkown Argument Provided\n\n");
            errorMessage();
        }
    }
    // this avoid errors when program ran only with verbose flag
    if (config->netid == NULL)
        errorMessage();

    log_debug("END of parser reached\n");
}
// function pointer to receive user input with null termineted char
void received_input(char *data) {
    // You have received input from the user insterface
    // Convert input line into json
    char *jBuff = calloc(STATUS_BUF_SIZE, sizeof(char *));
    char *user = NULL;

    if (name == NULL)
        user = netid;
    else
        user = name;

    time_t now;
    struct tm ts;
    char curret_time[STATUS_BUF_SIZE];
    // Get current time
    time(&now);
    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    ts = *localtime(&now);
    strftime(curret_time, sizeof(curret_time), "%H:%M:%S", &ts);
    log_debug("Message: %s", data);
    jBuff = json_asprintf("{timestamp: %d, name: %Q, message: %Q }", curret_time, netid, data);
    log_info("Jason Message: %s", jBuff);
    // Publish data
    publish_message(jBuff, false);
    free(jBuff);
} // subscribe to the topic passed in config
// initialize connection and send online publication
void chat_init(Config *config) {
    int rc;
    // set the address passed in config
    char *address = (char *)malloc(8 + strlen(config->host) + strlen(config->port));
    strcpy(address, config->host);
    strcat(address, ":");
    strcat(address, config->port);
    log_info("Address: %s", address);

    MQTTClient_willOptions will = MQTTClient_willOptions_initializer;
    will.message = json_asprintf("{name: %Q, online: %d}", name, CLIENT_OFFLINE);
    char *status_topic = calloc(strlen(netid) + 4, sizeof(char *));
    strcpy(status_topic, netid);
    strcat(status_topic, "/");
    strcat(status_topic, CHAT_PUBLISH_STATUS);
    strcat(status_topic, "\0");
    will.topicName = status_topic;
    will.retained = MQTT_PUB_QOS;
    will.qos = MQTT_PUB_QOS;
    conn_opts.will = &will;
    MQTTClient_create(&client, address, netid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = MQTT_PUB_QOS;

    MQTTClient_setCallbacks(client, NULL, connlost, messages, delivered);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to connect, code:  %d\n", rc);
        free(address);
        exit(EXIT_FAILURE);
    }
    // subscribe to the 2 main topics to start.
    log_info("subscription to +/status and +/message");
    // subscribe to status topic
    MQTTClient_subscribe(client, CHAT_STATUS_TOPIC, MQTT_SUB_QOS);
    // subscribe to message topic
    MQTTClient_subscribe(client, CHAT_MESSAGE_TOPIC, MQTT_SUB_QOS);
    log_info("Done with subscribtions");
    // publish the start message with the online status
    onlineStatus();
    free(address);
    // free(status_topic);
    log_debug("Done with CHAT INIT\n");
}
// publish to a topic if TRUE- it is status topic
void publish_message(char *jsonMessage, bool retained) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    // message
    pubmsg.payload = jsonMessage;
    pubmsg.payloadlen = strlen(jsonMessage);
    pubmsg.qos = MQTT_PUB_QOS;
    pubmsg.retained = retained;
    deliveredtoken = 0;
    // create the publish topic with the net id
    char *publish_topic = NULL;
    if (retained) {
        publish_topic = (char *)malloc(8 + strlen(netid) + strlen(CHAT_PUBLISH_STATUS));
        strcpy(publish_topic, netid);
        strcat(publish_topic, "/");
        strcat(publish_topic, CHAT_PUBLISH_STATUS);
        strcat(publish_topic, "\0"); // add the null char
        MQTTClient_publishMessage(client, publish_topic, &pubmsg, &token);
    } else {
        publish_topic = (char *)malloc(8 + strlen(netid) + strlen(CHAT_PUBLISH_MESSAGE));
        strcpy(publish_topic, netid);
        strcat(publish_topic, "/");
        strcat(publish_topic, CHAT_PUBLISH_MESSAGE);
        strcat(publish_topic, "\0"); // add the null char
        MQTTClient_publishMessage(client, publish_topic, &pubmsg, &token);
    }
    log_debug("Published topic: %s", publish_topic);
    log_debug("Json Sent: %s", jsonMessage);
    // log_debug("Waiting publication");
    while (deliveredtoken != token)
        ;
    free(publish_topic);
}
//  STATUS FUNCTIONS
void onlineStatus() {
    // format to receive message
    log_info("Start of online Status\n");
    char *jBuff = calloc(STATUS_BUF_SIZE, sizeof(char *));
    char *user = NULL;
    if (name == NULL)
        user = netid;
    else
        user = name;
    jBuff = json_asprintf("{name: %Q, online: %d}", user, CLIENT_ONLINE);
    log_info("Json Online status: %s", jBuff);
    // publish the online message
    publish_message(jBuff, true);
    free(jBuff);
}
void offlineStatus() {
    char *jBuff = calloc(STATUS_BUF_SIZE, sizeof(char *));
    char *user = NULL;
    if (name == NULL)
        user = netid;
    else
        user = name;
    jBuff = json_asprintf("{name: %Q, online: %d}", user, CLIENT_OFFLINE);
    log_debug("Json OFFLINE status: %s", jBuff);
    publish_message(jBuff, true);
    free(jBuff);
}
// close the connections
void close_connection() {
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}
/****************************************************************************
 *                  ERROR CHECKING FUNCTIONS
 * *************************************************************************/
/**
 * @brief  check if a port is valid or not
 * @param  port: check if valid
 * @retval  true for a valid port. false otherwise
 */
bool checkPort(char *port) {
    for (unsigned long i = 0; i < strlen(port); i++) {
        if (!isdigit(port[i])) // not digit in port
            return true;
    }
    return false; // port is just numbers
}
/**
 * @brief  print help message to stdout
 * @retval None
 */
void helpMessage() {
    log_debug("Help Message Function called");
    fprintf(stdout, "Usage: chat [--help] [-v] [-h HOST] [-p PORT] [-n NAME] NETID \n\n");
    fprintf(stdout, "Arguments:\n  NETID The NetID of the user. \n");
    fprintf(stdout, "Options:\n  --help\n  -v, --verbose\n  --host HOSTNAME, -h "
                    "HOSTNAME\n  --port PORT, -p PORT\n  --name NAME, -n NAME \n");
    exit(EXIT_SUCCESS); // display message and exit
}
/**
 * @brief  print error help message to stderr
 * @retval None
 */
void errorMessage() {
    log_debug("Error Function called\n");
    fprintf(stderr, "Usage: chat [--help] [-v] [-h HOST] [-p PORT] [-n NAME] NETID \n\n");
    fprintf(stderr, "Arguments:\n  NETID The NetID of the user. \n");
    fprintf(stderr, "Options:\n  --help\n  -v, --verbose\n  --host HOSTNAME, -h "
                    "HOSTNAME\n  --port PORT, -p PORT\n  --name NAME, -n NAME \n");
    exit(EXIT_FAILURE);
}
