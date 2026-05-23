#include "memory.h"

#define KERNEL_HEAP_SIZE (1024 * 1024)

typedef struct heap_block {
    unsigned int size;
    int free;
    struct heap_block *next;
} heap_block_t;

static unsigned char kernel_heap[KERNEL_HEAP_SIZE];

static heap_block_t *heap_head = 0;

void heap_init(void)
{
    heap_head = (heap_block_t*)kernel_heap;

    heap_head->size =
        KERNEL_HEAP_SIZE - sizeof(heap_block_t);

    heap_head->free = 1;
    heap_head->next = 0;
}

static void split_block(
    heap_block_t *block,
    unsigned int size
)
{
    heap_block_t *new_block =
        (heap_block_t*)
        ((unsigned char*)block +
        sizeof(heap_block_t) +
        size);
    
    new_block->size =
        block->size -
        size -
        sizeof(heap_block_t);
    
    new_block->free = 1;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;
}

void *kmalloc(unsigned int size)
{
    heap_block_t *current = heap_head;

    // 8-byte alignment
    if (size % 8 != 0) {
        size += 8 - (size % 8);
    }

    while (current) {
        if (current->free &&
            current->size >= size)
        {
            // Split if large enough
            if (current->size >
                size + sizeof(heap_block_t))
            {
                split_block(current, size);
            }

            current->free = 0;

            return (unsigned char*)current + sizeof(heap_block_t);
        }

        current = current->next;
    }

    return 0;
}

static void merge_free_blocks(void)
{
    heap_block_t *current = heap_head;

    while (current && current->next) {
        if (current->free &&
            current->next->free)
        {
            current->size +=
                sizeof(heap_block_t) +
                current->next->size;

            current->next =
                current->next->next;
        } else {
            current = current->next;
        }
    }
}

void kfree(void *ptr)
{
    if (!ptr) {
        return;
    }

    heap_block_t *block =
        (heap_block_t*)
        ((unsigned char*)ptr -
        sizeof(heap_block_t));

    block->free = 1;

    merge_free_blocks();
}