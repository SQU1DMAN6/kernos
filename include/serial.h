#ifndef KERNOS_SERIAL_H
#define KERNOS_SERIAL_H

#include <stdint.h>

void serial_init (void);
void serial_write_char (char c);
void serial_write_string (const char *str);
void serial_write_hex32 (uint32_t value);
void serial_write_hex64 (uint64_t value);

#endif
