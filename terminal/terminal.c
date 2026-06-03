#include "terminal.h"
#include "keyboard_map.h"
#include "shell.h"
#include "string.h"
#include "framebuffer.h"
#include "timer.h"
#include "kernos8x16.h"
#include <stddef.h>
#include <stdint.h>

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

static const char *PRIMARY_PROMPT = "=> ";
static const char *CONTINUATION_PROMPT = " > ";

static uint32_t fg_colour = 0xFFFFFF;
static uint32_t bg_colour = 0x000000;

static unsigned int cursor_visible = 1;
static unsigned int last_cursor_tick = 0;

static int terminal_dirty = 1;

static int prev_cursor_x = 0;
static int prev_cursor_y = 0;

extern void write_port(unsigned short port, unsigned char data);

void terminal_set_colour(uint32_t fg, uint32_t bg)
{
    fg_colour = fg;
    bg_colour = bg;
}

static void draw_char_fb(int cx, int cy, char c)
{
    if ((unsigned char)c >= 256)
        return;

    int px = cx * KERNOS_FONT_WIDTH;
    int py = cy * KERNOS_FONT_HEIGHT;

    const uint8_t *glyph = kernos_font8x16[(uint8_t)c];

    for (int y = 0; y < KERNOS_FONT_HEIGHT; y++) {
        uint8_t row = glyph[y];

        for (int x = 0; x < KERNOS_FONT_WIDTH; x++) {
            uint32_t colour = (row & (0x80 >> x)) ? fg_colour : bg_colour;
            put_pixel(px + x, py + y, colour);
        }
    }
}

static void draw_cursor_fb(int cx, int cy)
{
    int px = cx * KERNOS_FONT_WIDTH;
    int py = cy * KERNOS_FONT_HEIGHT;

    for (int y = KERNOS_FONT_HEIGHT - 2;
        y < KERNOS_FONT_HEIGHT;
        y++)
    {
        for (int x = 0;
            x < KERNOS_FONT_WIDTH;
            x++)
        {
            put_pixel(px + x, py + y, fg_colour);
        }
    }
}

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

    // Clear the bottom row
    for (unsigned int x = 0; x < TERM_W; x++) {
        term[TERM_H - 1][x] = 0;
    }

    // Cursor stays at bottom line
    cursor_y = VISIBLE_ROWS - 1;

    terminal_dirty = 1;
    update_cursor();
}

void term_put_char(char c)
{
    if (cursor_x >= TERM_W) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VISIBLE_ROWS) {
        scroll_terminal();
        cursor_y = VISIBLE_ROWS - 1;
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
        terminal_dirty = 1;
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

    terminal_dirty = 1;
    redraw_input_line();
}

void term_del(void)
{
    if (input_cursor >= input_length) {
        return;
    }

    for (unsigned int i = input_cursor;
        i < input_length - 1;
        i++)
    {
        input_buffer[i] = input_buffer[i + 1];
    }

    input_length--;

    input_buffer[input_length] = 0;

    terminal_dirty = 1;
    redraw_input_line();
}

void render_terminal()
{
    if (!terminal_dirty) return;

    for (unsigned int y = 0; y < TERM_H; y++) {
        for (unsigned int x = 0; x < TERM_W; x++) {
            draw_char_fb(x, y, term[y][x]);
        }
    }

    terminal_dirty = 0;
}

void render_cursor(void)
{
    // Erase the previous cursor by redrawing that cell
    draw_char_fb(
        prev_cursor_x,
        prev_cursor_y,
        term[prev_cursor_y][prev_cursor_x]
    );

    if ((timer_ticks % 50) < 25) {
        draw_cursor_fb(cursor_x, cursor_y);
    }

    if ((timer_ticks % 50) > 25) {
        terminal_dirty = 1;
    }

    prev_cursor_x = cursor_x;
    prev_cursor_y = cursor_y;
}

void kprintln(void)
{
    cursor_x = 0;
    cursor_y++;

    if (cursor_y >= TERM_H) {
        scroll_terminal();
        cursor_y = VISIBLE_ROWS - 1;
    }

    update_cursor();
}

void kprint(const char *str)
{
    terminal_set_colour(0xFFFFFF, 0x000000);

    unsigned int i = 0;
    while (str[i] != '\0') {
        term_put_char(str[i++]);
    }

    terminal_dirty = 1;
}

void kprint_uint(uint32_t value)
{
    char buf[11]; // max: 4294967295 + null
    int i = 10;
    buf[i] = '\0';

    if (value == 0) {
        kprint("0");
        return;
    }

    while (value > 0 && i > 0) {
        buf[--i] = '0' + (value % 10);
        value /= 10;
    }

    kprint(&buf[i]);
}

void shell_prompt(void)
{
    prompt_x = cursor_x;
    prompt_y = cursor_y;

    kprint(PRIMARY_PROMPT);
}

void clear_screen(void)
{
    // Clear terminal buffer
    for (unsigned int y = 0; y < TERM_H; y++) {
        for (unsigned int x = 0; x < TERM_W; x++) {
            term[y][x] = 0;
        }
    }

    // Clear framebuffer
    for (unsigned int y = 0; y < framebuffer.height; y++) {
        for (unsigned int x = 0; x < framebuffer.width; x++) {
            put_pixel(x, y, bg_colour);
        }
    }

    // Reset cursor position
    cursor_x = 0;
    cursor_y = 0;

    update_cursor();
}

void clear_input_line(void) {
    unsigned int total =
        3 + input_length;

    unsigned int lines =
        (prompt_x + total) / TERM_W + 1;

    for (unsigned int y = 0;
        y < lines;
        y++)
    {
        unsigned int row =
            prompt_y + y;

        if (row >= TERM_H) {
            break;
        }

        for (unsigned int x = 0;
            x < TERM_W;
            x++)
        {
            term[row][x] = 0;
        }
    }

    cursor_x = prompt_x;
    cursor_y = prompt_y;

    update_cursor();
}

void redraw_input_line(void)
{
    clear_input_line();

    terminal_dirty = 1;

    cursor_x = prompt_x;
    cursor_y = prompt_y;

    unsigned int x = prompt_x;
    unsigned int y = prompt_y;

    for (unsigned int i = 0;
        PRIMARY_PROMPT[i];
        i++)
    {
        term[y][x++] = PRIMARY_PROMPT[i];
    }

    for (unsigned int i = 0;
        i < input_length;
        i++)
    {
        if (x >= TERM_W) {
            x = 0;
            y++;

            if (y >= TERM_H) {
                scroll_terminal();
                y = TERM_H - 1;
            }

            x = 0;

            for (unsigned int j = 0;
                CONTINUATION_PROMPT[j];
                j++)
            {
                term[y][x++] =
                    CONTINUATION_PROMPT[j];
            }
        }

        if (y >= TERM_H) {
            break;
        }

        if (x >= TERM_W) {
            continue;
        }

        term[y][x++] =
            input_buffer[i];
    }

    x = prompt_x + strlen(PRIMARY_PROMPT);
    y = prompt_y;

    for (unsigned int i = 0;
        i < input_cursor;
        i++)
    {
        x++;

        if (x >= TERM_W) {
            y++;
            x = strlen(PRIMARY_PROMPT);
        }
    }

    cursor_x = x;
    cursor_y = y;

    update_cursor();
}
