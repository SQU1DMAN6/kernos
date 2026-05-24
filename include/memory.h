#ifndef MEMORY_H
#define MEMORY_H

void heap_init(void);

void *kmalloc(unsigned int size);
void kfree(void *ptr);

unsigned int heap_total(void);
unsigned int heap_used(void);
unsigned int heap_free(void);

#endif
