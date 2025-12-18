#ifdef NINTENDOWII
#ifdef FFMPEG_INCLUDED
#include <gccore.h>
#include <ogc/system.h>

#include <stdint.h>
#include <string.h>
#include "ffmpeg_mem2_alloc.h"

// Manual prototype to silence implicit declaration (optional if gccore.h is included)
void* SYS_AllocArena2MemLo(uint32_t size, uint32_t align);

#define FFMPEG_MEM2_SIZE (32 * 1024 * 1024)
#define ALIGNMENT 32

typedef struct Block {
    size_t size;
    struct Block *next;
} Block;

static uint8_t *heap_base = NULL;
static Block *free_list = NULL;

#define ALIGN(x) (((x) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define HEADER_SIZE ALIGN(sizeof(Block))

// Initialize allocator
void ffmpeg_allocator_init(void)
{
    heap_base = (uint8_t*)SYS_AllocArena2MemLo(FFMPEG_MEM2_SIZE, ALIGNMENT);
    free_list = (Block*)heap_base;

    free_list->size = FFMPEG_MEM2_SIZE - HEADER_SIZE;
    free_list->next = NULL;
}

// Reset allocator (reclaims all memory)
void ffmpeg_allocator_reset(void)
{
    if (!heap_base) return;
    free_list = (Block*)heap_base;
    free_list->size = FFMPEG_MEM2_SIZE - HEADER_SIZE;
    free_list->next = NULL;
}

// Allocate memory
void *ffmpeg_malloc(size_t size)
{
    size = ALIGN(size);

    Block **curr = &free_list;
    while (*curr)
    {
        if ((*curr)->size >= size)
        {
            Block *block = *curr;

            if (block->size > size + HEADER_SIZE)
            {
                Block *new_block = (Block*)((uint8_t*)block + HEADER_SIZE + size);
                new_block->size = block->size - size - HEADER_SIZE;
                new_block->next = block->next;
                *curr = new_block;
            }
            else
            {
                *curr = block->next;
            }

            block->size = size;
            return (uint8_t*)block + HEADER_SIZE;
        }
        curr = &(*curr)->next;
    }

    return NULL; // Out of memory
}

// Free memory (adds back to free list)
void ffmpeg_free(void *ptr)
{
    if (!ptr) return;

    Block *block = (Block*)((uint8_t*)ptr - HEADER_SIZE);
    block->next = free_list;
    free_list = block;
}

// Allocate zeroed memory
void *ffmpeg_calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *ptr = ffmpeg_malloc(total);
    if (ptr)
        memset(ptr, 0, total);
    return ptr;
}

// Reallocate memory (always returns new allocation)
void *ffmpeg_realloc(void *ptr, size_t size)
{
    if (!ptr)
        return ffmpeg_malloc(size);

    void *new_ptr = ffmpeg_malloc(size);
    if (new_ptr)
    {
        // Copy old data (up to old block size)
        Block *block = (Block*)((uint8_t*)ptr - HEADER_SIZE);
        size_t copy_size = (size < block->size) ? size : block->size;
        memcpy(new_ptr, ptr, copy_size);
        ffmpeg_free(ptr);
    }
    return new_ptr;
}
#else // FFMPEG_INCLUDED

#include <stddef.h>

// Stub functions (They dont work)
void ffmpeg_allocator_init(void) {
    return;
}
void ffmpeg_allocator_reset(void) {
    return;
}
void *ffmpeg_malloc(size_t size) {
    return NULL;
}
void ffmpeg_free(void *ptr) {
    return;
}
void *ffmpeg_calloc(size_t nmemb, size_t size) {
    return NULL;
}
void *ffmpeg_realloc(void *ptr, size_t size) {
    return NULL;
}

#endif // FFMPEG_INCLUDED
#endif // NINTENDOWII
