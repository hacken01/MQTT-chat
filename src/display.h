#include <ncurses.h>

// Sets up ncurses.
int display_init();

// This function takes input from the user keyboard and displays it in the
// input window. When the user presses enter, the input_func gets called with
// the input string as the parameter. This function is blocking ahd should be
// called last in your main.
int display_process_input(void (*input_func)(char *));

// Used to display a line of text in your chat window.
void display_line();