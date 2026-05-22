#ifndef IO_H
#define IO_H

extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

#endif /* IO_H */
