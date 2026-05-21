#include "keyboard_map.h"

// There are 25 lines of 80 columns; each element takes up 2 bytes.
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C
#define BACKSPACE_KEY_CODE 0x0E

#define LEFT_SHIFT_PRESS 0x2A
#define LEFT_SHIFT_RELEASE 0xAA

#define RIGHT_SHIFT_PRESS 0x36
#define RIGHT_SHIFT_RELEASE 0xB6

#define CAPSLOCK_KEY_CODE 0x3A

#define ARROW_UP_PRESS 0x48
#define ARROW_DOWN_PRESS 0x50

#define ARROW_LEFT_PRESS 0x4B
#define ARROW_RIGHT_PRESS 0x4D

extern unsigned char keyboard_map[128];
extern unsigned char keyboard_shift_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

// Current cursor location
unsigned int current_loc = 0;
// Video memory begins at address 0xb8000
char *vidptr = (char*)0xb8000;

// Terminal buffer layer
#define TERM_W 80
#define TERM_H 25

static char term[TERM_H][TERM_W];
static unsigned int term_x = 0;
static unsigned int term_y = 0;

static unsigned int cursor_x = 0;
static unsigned int cursor_y = 0;

static unsigned int prompt_x = 0;
static unsigned int prompt_y = 0;

static int left_shift_pressed = 0;
static int right_shift_pressed = 0;
static int caps_lock_enabled = 0;

#define INPUT_BUFFER_SIZE 256
#define HISTORY_SIZE 32

static char input_buffer[INPUT_BUFFER_SIZE];
static unsigned int input_length = 0;
static unsigned int input_cursor = 0;

static char command_history[HISTORY_SIZE][INPUT_BUFFER_SIZE];
static unsigned int history_count = 0;
static int history_index = -1;

// Forward declarations
void redraw_input_line(void);

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

struct IDT_entry {
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

void idt_init(void)
{
    unsigned long keyboard_address;
    struct idt_ptr {
        unsigned short limit;
        unsigned int base;
    } __attribute__((packed));

    struct idt_ptr idtp;

    // Populate the IDT entry of keyboard's interrupt
    keyboard_address = (unsigned long)keyboard_handler;
    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = INTERRUPT_GATE;
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

    /* Ports
    *          PIC1    PIC2
    *  Command 0x20    0xA0
    *  Data    0x21    0xA1
    */

    // ICW1 - begin initialisation
    write_port(0x20, 0x11);
    write_port(0xA0, 0x11);

    // ICW2 - remap the offset address of IDT.
    // In x86 protected mode, we have to remap the PICs beyond 0x20 because
    // Intel has designated the first 32 interrupts as "reserved" for CPU exceptions.
    write_port(0x21, 0x20);
    write_port(0xA1, 0x28);

    // ICW3 - setup cascading.
    // Master ICW3 is a bitmask of attached slave IRQ lines. The slave is on
    // IRQ2, so bit 2 must be set. Slave ICW3 is its cascade identity: IRQ2.
    write_port(0x21, 0x04);  // Master: slave is on IRQ2
    write_port(0xA1, 0x02);  // Slave: cascade identity = 2

    // ICW4 - environment information (8086/88 mode)
    write_port(0x21, 0x01);
    write_port(0xA1, 0x01);

    // Initialisation finished

    // Fill the IDT descriptor
    idtp.limit = (sizeof(struct IDT_entry) * IDT_SIZE) - 1;
    idtp.base = (unsigned int)&IDT;

    write_port(0x21, 0xFF);
    write_port(0xA1, 0xFF);

    load_idt((unsigned long*)&idtp);
}

void kb_init(void)
{
    // Enable keyboard IRQ1 on master (bit 1 cleared), mask all slave IRQs.
    write_port(0x21, 0xFD);
    write_port(0xA1, 0xFF);
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
    while (i < SCREENSIZE) {
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

    for (unsigned int x = prompt_x;
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
        unsigned int x = prompt_x + 3 + i;
        if (x >= TERM_W) {
            break;
        }

        term[prompt_y][x] = input_buffer[i];
    }

    cursor_x = prompt_x + 3 + input_cursor;
    cursor_y = prompt_y;

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
            command_history[history_count % HISTORY_SIZE],
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
        command_history[index % HISTORY_SIZE]
    );

    input_length = 0;

    while (input_buffer[input_length]) {
        input_length++;
    }

    redraw_input_line();
}

void keyboard_handler_main(void)
{
    unsigned char status;
    unsigned char keycode;
    char character;

    status = read_port(KEYBOARD_STATUS_PORT);
    if ((status & 0x01) == 0)
        return;

    keycode = read_port(KEYBOARD_DATA_PORT);

    if (keycode == LEFT_SHIFT_PRESS) {
        left_shift_pressed = 1;
        return;
    }

    if (keycode == RIGHT_SHIFT_PRESS) {
        right_shift_pressed = 1;
        return;
    }

    if (keycode == LEFT_SHIFT_RELEASE) {
        left_shift_pressed = 0;
        return;
    }

    if (keycode == RIGHT_SHIFT_RELEASE) {
        right_shift_pressed = 0;
        return;
    }

    if (keycode == CAPSLOCK_KEY_CODE) {
        caps_lock_enabled = !caps_lock_enabled;
        return;
    }

    if (keycode == ARROW_UP_PRESS) {
        if (history_count > 0) {
            if (history_index == -1) {
                history_index = history_count;
            }

            if (history_index > 0) {
                history_index--;
                load_history_entry(history_index);
            }
        }

        return;
    }

    if (keycode == ARROW_DOWN_PRESS) {
        if (history_index < (int)history_count - 1) {
            history_index++;
            load_history_entry(history_index);
        } else {
            history_index = history_count;

            input_length = 0;
            input_buffer[0] = 0;

            redraw_input_line();
        }

        return;
    }

    if (keycode == ARROW_LEFT_PRESS) {
        if (input_cursor > 0) {
            input_cursor--;
            redraw_input_line();
        }

        return;
    }

    if (keycode == ARROW_RIGHT_PRESS) {
        if (input_cursor < input_length) {
            input_cursor++;
            redraw_input_line();
        }

        return;
    }

    if (keycode & 0x80)
        return;

    if (keycode == ENTER_KEY_CODE) {
        execute_command();
        return;
    }

    if (keycode == BACKSPACE_KEY_CODE) {
        term_backspace();
        return;
    }

    if (keycode >= 128)
        return;
    
    if (left_shift_pressed || right_shift_pressed) {
        character = keyboard_shift_map[keycode];
    } else {
        character = keyboard_map[keycode];
    }

    // Caps Lock affects alphabetic characters only
    if (caps_lock_enabled &&
        character >= 'a' &&
        character <= 'z')
    {
        character -= 32;
    } else if (
        caps_lock_enabled &&
        character >= 'A' &&
        character <= 'Z'
    )
    {
        character += 32;
    }

    if (character) {
        if (input_length < INPUT_BUFFER_SIZE - 1) {
            // Shift characters rightward
            for (int i = input_length;
                i > (int)input_cursor;
                i--)
            {
                input_buffer[i] = input_buffer[i - 1];
            }

            input_buffer[input_cursor] = character;

            input_length++;
            input_cursor++;

            input_buffer[input_length] = 0;

            redraw_input_line();
        }
    }
}

void kmain(void)
{
    clear_screen();
    for (int y = 0; y < TERM_H; y++) 
        for (int x = 0; x < TERM_W; x++)
            term[y][x] = 0;

    kprint("Booting Kernos...\n");

    idt_init();

    kprint("[  OK  ] IDT initialisation successful\n");

    __asm__ __volatile__("sti");

    kprint("[  OK  ] System is stable\n");

    kb_init();

    kprint("[  OK  ] Keyboard initialisation successful\n");

    kprint("         Welcome to Kernos, written by Quan Thai\n");

    shell_prompt();

    while(1) {
        render_terminal();
        __asm__("hlt");
    }
}
