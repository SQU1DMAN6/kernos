#include "terminal.h"
#include "shell.h"
#include "string.h"
#include "power.h"
#include "vfs.h"

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
        kprint("    help:   Show available commands\n");
        kprint("    clear:  Clear the terminal\n");
        kprint("    reboot: Reboot the system\n");
        kprint("    ls:     List files\n");
        kprint("    new:    Create file\n");
        kprint("    write:  Write text contents to a file\n");
        kprint("    cat:    Read file\n");
    }
    else if (strcmp(input_buffer, "clear")) {
        clear_screen();
    }
    else if (strcmp(input_buffer, "reboot")) {
        kprint("Rebooting...\n");
        reboot();
    }
    else if (strcmp(input_buffer, "ls")) {
        fs_list();
    }
    // These are temporary hard-coded tests. Until argument parsing is a thing, just keep it this way for now.
    else if (strcmp(input_buffer, "new")) {
        if (fs_create("RAM:/test.txt")) {
            kprint("Created test.txt\n");
        }
    }
    else if (strcmp(input_buffer, "write")) {
        if (fs_write(
            "RAM:/test.txt",
            "Hello from Kernos"
        ))
        {
            kprint("Write was successful\n");
        }
    }
    else if (strcmp(input_buffer, "cat")) {
        char *data =
            fs_read("RAM:/test.txt");

        if (data) {
            kprint(data);
            kprintln();
        }
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