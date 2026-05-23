#ifndef TERMINAL_H
#define TERMINAL_H

#define TERM_W 80
#define TERM_H 25
#define INPUT_BUFFER_SIZE 256

extern char *vidptr;
extern char term[TERM_H][TERM_W];
extern unsigned int cursor_x;
extern unsigned int cursor_y;

extern char input_buffer[INPUT_BUFFER_SIZE];
extern unsigned int input_length;
extern unsigned int input_cursor;
extern unsigned int history_count;
extern int history_index;

void update_cursor(void);
void render_terminal(void);
void term_put_char(char c);
void term_backspace(void);
void term_del(void);
void kprintln(void);
void kprint(const char *str);
void shell_prompt(void);
void clear_screen(void);
void clear_input_line(void);
void redraw_input_line(void);
void execute_command(void);
void load_history_entry(int index);

#endif /* TERMINAL_H */
