#include "keyboard.h"
#include "io.h"
#include "keyboard_map.h"
#include "terminal.h"

// Keyboard state
static int left_shift_pressed = 0;
static int right_shift_pressed = 0;
static int caps_lock_enabled = 0;

static int extended_scancode = 0;

void keyboard_handler_main(void)
{
    unsigned char status;
    unsigned char keycode;
    char character;

    status = read_port(KEYBOARD_STATUS_PORT);
    if ((status & 0x01) == 0)
        return;

    keycode = read_port(KEYBOARD_DATA_PORT);
    if (keycode == 0xE0) {
        extended_scancode = 1;
        return;
    }

    if (extended_scancode) {
        extended_scancode = 0;

        switch (keycode) {
            case ARROW_LEFT_PRESS:
                if (input_cursor > 0) {
                    input_cursor--;
                    redraw_input_line();
                }
                return;
            
            case ARROW_RIGHT_PRESS:
                if (input_cursor < input_length) {
                    input_cursor++;
                    redraw_input_line();
                }
                return;
            
            case ARROW_UP_PRESS:
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
            
            case ARROW_DOWN_PRESS:
                if (history_index < (int)history_count - 1) {
                    history_index++;
                    load_history_entry(history_index);
                } else {
                    history_index = history_count;

                    input_length = 0;
                    input_cursor = 0;
                    input_buffer[0] = 0;

                    redraw_input_line();
                }
                return;
        }
    }

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
