#include <stdlib.h>
#include <stdio.h>

#include "./arena.h"

#define __cearch_arena_alignment (sizeof(void*))

cearch_arena_t *cearch_arena_create(size_t capacity) {
    cearch_arena_t *arena = malloc(sizeof(cearch_arena_t));

    if (!arena) return NULL;

    arena->buffer = malloc(capacity);

    if (!arena->buffer) {
        free(arena);
        return NULL;
    }

    arena->capacity = capacity;
    arena->offset = 0;

    return arena;
}

void *cearch_arena_alloc(cearch_arena_t *arena, size_t size) {
    size_t padding = 0;
    uintptr_t current_ptr = (uintptr_t)(arena->buffer + arena->offset);
    uintptr_t remainder = current_ptr % __cearch_arena_alignment;

    if (remainder != 0) {
        padding = __cearch_arena_alignment - remainder;
    }

    if (arena->offset + padding + size > arena->capacity) {
        fprintf(stderr, "[CEARCH_ARENA_ERROR]: Arena ran out of memory\n");
        exit(1);
    }

    void *allocated_ptr = arena->buffer + arena->offset + padding;

    arena->offset += padding + size;

    return allocated_ptr;
}

inline void cearch_arena_reset(cearch_arena_t *arena) {
    arena->offset = 0;
}

inline void cearch_arena_destroy(cearch_arena_t *arena) {
    if (arena) {
        free(arena->buffer);
        free(arena);
    }
}
