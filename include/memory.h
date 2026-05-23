#ifndef MEMORY_H
#define MEMORY_H

void heap_init(void);

void *kmalloc(unsigned int size);
void kfree(void *ptr);

#endif