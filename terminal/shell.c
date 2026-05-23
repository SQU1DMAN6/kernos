#include "terminal.h"
#include "shell.h"
#include "string.h"
#include "power.h"
#include "vfs.h"

extern char command_history[32][INPUT_BUFFER_SIZE];

int parse_arguments(
    char *input,
    char *argv[]
)
{
    int argc = 0;
    int i = 0;

    while (input[i]) {
        while (input[i] == ' ') {
            i++;
        }

        if (!input[i]) {
            break;
        }

        argv[argc++] = &input[i];

        while (input[i] && input[i] != ' ') {
            i++;
        }

        if (input[i]) {
            input[i] = 0;
            i++;
        }

        if (argc >= MAX_ARGUMENTS) {
            break;
        }
    }

    return argc;
}

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

    char *argv[MAX_ARGUMENTS];

    int argc =
        parse_arguments(
            input_buffer,
            argv
        );

    if (argc == 0) {
        shell_prompt();
        return;
    }

    if (strcmp(argv[0], "help")) {
        kprint("Builtin commands:\n");
        kprint("    help:   Show available commands\n");
        kprint("    clear:  Clear the terminal\n");
        kprint("    echo:   Print out text\n");
        kprint("    reboot: Reboot the system\n");
        kprint("    ls:     List files\n");
        kprint("    new:    Create file\n");
        kprint("    write:  Write text contents to a file\n");
        kprint("    cat:    Read file\n");
        kprint("    mkd:    Make directory\n");
    }
    else if (strcmp(argv[0], "clear")) {
        clear_screen();
    }
    else if (strcmp(argv[0], "reboot")) {
        kprint("Rebooting...\n");
        reboot();
    }
    else if (strcmp(argv[0], "ls")) {
        if (argc > 2) {
            kprint("Usage: ls <dir>\n");
        }
        else if (argc == 1) {
            fs_list();
        } else if (argc == 2) {
            fs_list_path(argv[1]);
        }
    }
    // These are temporary hard-coded tests. Until argument parsing is a thing, just keep it this way for now.
    else if (strcmp(argv[0], "new")) {
        if (argc < 2) {
            kprint("Usage: new <name>\n");
        }
        else if (fs_create(argv[1])) {
            kprint("[  OK  ] NEW ");
            kprint(argv[1]);
            kprintln();
        }
    }
    else if (strcmp(argv[0], "mkd")) {
        if (argc < 2) {
            kprint("Usage: mkd <name>\n");
        }
        else if (fs_mkdir(argv[1])) {
            kprint("[  OK  ] MKD ");
            kprint(argv[1]);
            kprintln();
        }
        else {
            kprint("[FAILED] MKD ");
            kprint(argv[1]);
            kprintln();
        }
    }
    else if (strcmp(argv[0], "write")) {
        if (argc < 3) {
            kprint("Usage: write <name> <content>\n");
        }
        else if (fs_write(
            argv[1],
            argv[2]
        ))
        {
            kprint("[  OK  ] WRITE ");
            kprint(argv[1]);
            kprintln();
        }
    }
    else if (strcmp(argv[0], "echo")) {
        for (int i = 1; i < argc; i++) {
            kprint(argv[i]);

            if (i != argc - 1) {
                kprint(" ");
            }
        }

        kprintln();
    }
    else if (strcmp(argv[0], "cat")) {
        if (argc != 2) {
            kprint("Usage: cat <path>\n");
        }
        else {
            char *data =
                fs_read(argv[1]);
            
            if (data) {
                kprint(data);
                kprintln();
            }
        }
    }
    else if (input_length != 0) {
        kprint("Unknown command: ");
        kprint(argv[0]);
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
