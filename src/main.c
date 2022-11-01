#include "chat.h"

// handle the exit signal
void signalHandler() {
    log_info("Handling Interrupt\n");
    fflush(stdout);
    offlineStatus();
    close_connection();
    exit(EXIT_SUCCESS);
}
////////////////////    MAIN FUNCTION    ///////////////////////////
////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    Config config;
    signal(SIGINT, signalHandler); // handle the ctl signal
    // Parse arguments
    parse_arguments(&config, argc, argv);
    log_info("Done with parse arguments\n");
    // Start display
    display_init();
    // Start MQTT
    chat_init(&config);
    // Receive input from user forever...
    display_process_input(received_input);
    return EXIT_SUCCESS;
}
