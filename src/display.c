#include "display.h"
#include "log.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INPUT_BUFFER_SIZE 1024
#define MAX_CHAT_WINDOW_SIZE 50

WINDOW *chat_window_border;
WINDOW *chat_window;
WINDOW *input_window_border;
WINDOW *input_window;
int chat_window_lines;

char input_data[INPUT_BUFFER_SIZE];
char chat_window_data[MAX_CHAT_WINDOW_SIZE * INPUT_BUFFER_SIZE];
char line_data[MAX_CHAT_WINDOW_SIZE][INPUT_BUFFER_SIZE];
int line_position;

int display_init() {
    log_trace("Setting up display...");

    // Set up ncurses
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    refresh();

    int lines;

    if (LINES > MAX_CHAT_WINDOW_SIZE) {
        lines = MAX_CHAT_WINDOW_SIZE;
    } else {
        lines = LINES;
    }

    // Set up data structure for windows
    chat_window_lines = lines - 5;

    // Add null terminators to every line
    for (int i = 0; i < chat_window_lines; i++) {
        line_data[i][0] = '\0';
    }

    chat_window_border = newwin(lines - 4, COLS, 0, 0);
    box(chat_window_border, 0, 0);
    wrefresh(chat_window_border);

    chat_window = newwin(lines - 6, COLS - 2, 1, 1);
    wrefresh(chat_window);

    input_window_border = newwin(4, COLS, lines - 4, 0);
    box(input_window_border, 0, 0);
    wrefresh(input_window_border);

    input_window = newwin(2, COLS - 2, lines - 3, 1);
    wrefresh(input_window);

    return 0;
}

int display_process_input(void (*input_func)(char *)) {
    log_trace("Getting ready to process input...");

    unsigned int input_position = 0;
    line_position = 0;
    int ch;

    while ((ch = getch())) {

        switch (ch) {
        case KEY_ENTER:
        case 10:
            if (input_position == 0) {
                // Ignore enters when no text has been entered.
                continue;
            }

            if (input_position > sizeof(input_data)) {
                // TODO: What should I do here?
            }

            // Null terminate string so I can publish it
            input_data[input_position] = '\0';
            input_position = 0;

            // Do something with the data!
            input_func(input_data);

            // Clear out input box
            werase(input_window);
            wrefresh(input_window);

            break;

        case KEY_BACKSPACE:
        case 127:
            input_position = input_position - 1 >= 0 ? input_position - 1 : 0;
            int x = getcurx(input_window);
            int y = getcury(input_window);
            wmove(input_window, y, --x);
            wdelch(input_window);
            wrefresh(input_window);
            break;

        case KEY_UP:
        case KEY_DOWN:
        case KEY_LEFT:
        case KEY_RIGHT:
            break;

        default:
            wechochar(input_window, ch);
            input_data[input_position] = ch;
            input_position++;
        }
    }

    endwin();

    return 0;
}

void display_line(char *line) {
    memcpy(line_data[line_position], line, strlen(line) + 1);
    line_position = (line_position + 1) % chat_window_lines;

    // Combine all the lines into one string
    chat_window_data[0] = '\0'; // "Clear" old data
    int read_pos = (line_position + 1) % chat_window_lines;
    while (read_pos != line_position) {
        strcat(chat_window_data, line_data[read_pos]);
        strcat(chat_window_data, "\n");
        read_pos = (read_pos + 1) % chat_window_lines;
    }

    // Display the data
    werase(chat_window);
    mvwprintw(chat_window, 0, 0, chat_window_data);
    wrefresh(chat_window);
}