#include "terminal.h"
#include "keyboard_map.h"

char *vidptr = (char*)0xb8000;

char term[TERM_H][TERM_W];
unsigned int term_x = 0;
unsigned int term_y = 0;

unsigned int cursor_x = 0;
unsigned int cursor_y = 0;

unsigned int prompt_x = 0;
unsigned int prompt_y = 0;

char input_buffer[INPUT_BUFFER_SIZE];
unsigned int input_length = 0;
unsigned int input_cursor = 0;

char command_history[32][INPUT_BUFFER_SIZE];
unsigned int history_count = 0;
int history_index = -1;

extern void write_port(unsigned short port, unsigned char data);

void update_cursor(void)
{
    unsigned short position = (cursor_y * TERM_W) + cursor_x;

    // Send high byte
    write_port(0x3D4, 14);
    write_port(0x3D5, (position >> 8) & 0xFF);

    // Send low byte
    write_port(0x3D4, 15);
    write_port(0x3D5, position & 0xFF);
}

void scroll_terminal(void)
{
    // Move every row up by one
    for (unsigned int y = 1; y < TERM_H; y++) {
        for (unsigned int x = 0; x < TERM_W; x++) {
            term[y - 1][x] = term[y][x];
        }
    }

    // Clear the final row
    for (unsigned int x = 0; x < TERM_W; x++) {
        term[TERM_H - 1][x] = 0;
    }

    cursor_y = TERM_H - 1;

    update_cursor();
}

void term_put_char(char c)
{
    if (cursor_y >= TERM_H) {
        scroll_terminal();
    }

    if (cursor_x >= TERM_W) {
        cursor_x = 0;
        cursor_y++;
    }

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        
        if (cursor_y >= TERM_H) {
            scroll_terminal();
        }

        update_cursor();
        return;
    }

    if (cursor_x >= TERM_W) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= TERM_H) {
        scroll_terminal();
    }

    if (cursor_y < TERM_H && cursor_x < TERM_W) {
        term[cursor_y][cursor_x] = c;
    }

    cursor_x++;

    update_cursor();
}

void redraw_input_line(void);

void term_backspace(void)
{
    if (input_cursor == 0) {
        return;
    }

    // Shift characters left
    for (unsigned int i = input_cursor - 1;
        i < input_length - 1;
        i++)
    {
        input_buffer[i] = input_buffer[i + 1];
    }

    input_length--;
    input_cursor--;

    input_buffer[input_length] = 0;

    redraw_input_line();
}

void render_terminal()
{
    unsigned int i = 0;

    for (unsigned int y = 0; y < TERM_H; y++) {
        for (unsigned int x = 0; x < TERM_W; x++) {
            char c = term[y][x];

            vidptr[i++] = (c >= 32 && c <= 126) ? c : ' ';
            vidptr[i++] = 0x1F;
        }
    }
}

void kprintln(void)
{
    cursor_x = 0;
    cursor_y++;

    if (cursor_y >= TERM_H) {
        scroll_terminal();
    }

    update_cursor();
}

void kprint(const char *str)
{
    unsigned int i = 0;
    while (str[i] != '\0') {
        term_put_char(str[i++]);
    }
}

void shell_prompt(void)
{
    prompt_x = cursor_x;
    prompt_y = cursor_y;

    kprint("=> ");
}

void clear_screen(void)
{
    // Clear terminal buffer
    for (unsigned int y = 0; y < TERM_H; y++) {
        for (unsigned int x = 0; x < TERM_W; x++) {
            term[y][x] = 0;
        }
    }

    // Clear VGA text memory
    unsigned int i = 0;
    while (i < (TERM_W * TERM_H * 2)) {
        vidptr[i++] = ' ';
        vidptr[i++] = 0x1f;
    }

    // Reset cursor position
    cursor_x = 0;
    cursor_y = 0;

    update_cursor();
}

int strcmp(const char *a, const char *b)
{
    unsigned int i = 0;

    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return 0;
        }

        i++;
    }

    return a[i] == b[i];
}

void strcpy(char *dest, const char *src)
{
    unsigned int i = 0;

    while (src[i]) {
        dest[i] = src[i];
        i++;
    }

    dest[i] = 0;
}

void clear_input_line(void) {
    cursor_x = prompt_x;
    cursor_y = prompt_y;

    for (unsigned int x = 0;
        x < TERM_W;
        x++)
    {
        term[prompt_y][x] = 0;
    }

    update_cursor();
}

void redraw_input_line(void)
{
    clear_input_line();

    cursor_x = prompt_x;
    cursor_y = prompt_y;

    term[prompt_y][prompt_x + 0] = '=';
    term[prompt_y][prompt_x + 1] = '>';
    term[prompt_y][prompt_x + 2] = ' ';

    for (unsigned int i = 0;
        i < input_length;
        i++)
    {
        unsigned int absolute =
            prompt_x + 3 + i;

        unsigned int x =
            absolute % TERM_W;

        unsigned int y = 
            prompt_y + (absolute / TERM_W);

        if (y >= TERM_H) {
            break;
        }

        term[y][x] = input_buffer[i];
    }

    unsigned int absolute_cursor =
        prompt_x + 3 + input_cursor;
    
    cursor_y = prompt_y + (absolute_cursor / TERM_W);
    cursor_x = absolute_cursor % TERM_W;

    if (cursor_x >= TERM_W) {
        cursor_x = TERM_W - 1;
    }

    update_cursor();
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
