#ifndef __cearch_arena_h_
#define __cearch_arena_h_

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
} cearch_arena_t;

cearch_arena_t *cearch_arena_create(size_t capacity);
void *cearch_arena_alloc(cearch_arena_t *arena, size_t size);
void cearch_arena_reset(cearch_arena_t *arena);
void cearch_arena_destroy(cearch_arena_t *arena);

#endif // __cearch_arena_h_
