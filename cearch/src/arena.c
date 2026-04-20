#include "./arena.h"

#include <stdlib.h>
#include <stdio.h>

#define CEARCH_ARENA_ALIGNMENT (sizeof(void*))

Cearch_Arena *arena_create(size_t capacity) {
    Cearch_Arena *arena = malloc(sizeof(Cearch_Arena));

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

void *arena_alloc(Cearch_Arena *arena, size_t size) {
    size_t padding = 0;
    uintptr_t current_ptr = (uintptr_t)(arena->buffer + arena->offset);
    uintptr_t remainder = current_ptr % CEARCH_ARENA_ALIGNMENT;

    if (remainder != 0) {
        padding = CEARCH_ARENA_ALIGNMENT - remainder;
    }

    if (arena->offset + padding + size > arena->capacity) {
        fprintf(stderr, "[CEARCH_ARENA_ERROR]: Arena ran out of memory\n");
        exit(1);
    }

    void *allocated_ptr = arena->buffer + arena->offset + padding;

    arena->offset += padding + size;

    return allocated_ptr;
}

void arena_reset(Cearch_Arena *arena) {
    arena->offset = 0;
}

void arena_destroy(Cearch_Arena *arena) {
    if (arena) {
        free(arena->buffer);
        free(arena);
    }
}
