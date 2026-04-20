#ifndef _CEARCH_ARENA_H
#define _CEARCH_ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
} Cearch_Arena;

Cearch_Arena* arena_create(size_t capacity);

void* arena_alloc(Cearch_Arena *arena, size_t size);

void arena_reset(Cearch_Arena *arena);

void arena_destroy(Cearch_Arena *arena);

#endif // _CEARCH_ARENA_H
