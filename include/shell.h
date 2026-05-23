#ifndef SHELL_H
#define SHELL_H

#define MAX_ARGUMENTS 64

void execute_command(void);
void load_history_entry(int index);

int parse_arguments(
    char *input,
    char *argv[]
);

#endif