#include "terminal.h"
#include "shell.h"
#include "string.h"

extern char command_history[32][INPUT_BUFFER_SIZE];

void execute_command(void)
{
    input_buffer[input_length] = 0;

    if (input_length > 0) {
        strcpy(
            command_history[history_count % 32],
            input_buffer
        );

        history_count++;
        history_index = history_count;
    }

    history_count = history_count;

    kprintln();

    if (strcmp(input_buffer, "help")) {
        kprint("Builtin commands:\n");
        kprint("    help:  Show available commands\n");
        kprint("    clear: Clear the terminal\n");
    }
    else if (strcmp(input_buffer, "clear")) {
        clear_screen();
    }
    else if (input_length != 0) {
        kprint("Unknown command: ");
        kprint(input_buffer);
        kprintln();
    }

    input_length = 0;
    input_cursor = 0;

    shell_prompt();
}

void load_history_entry(int index)
{
    if (index < 0 || index >= (int)history_count)
        return;
    
    strcpy(
        input_buffer,
        command_history[index % 32]
    );

    input_length = 0;

    while (input_buffer[input_length]) {
        input_length++;
    }

    input_cursor = input_length;

    redraw_input_line();
}