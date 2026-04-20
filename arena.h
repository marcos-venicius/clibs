#ifndef CLIBS_ARENA
#define CLIBS_ARENA

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
} Clibs_Arena;

Clibs_Arena* clibs_arena_create(size_t capacity);
void* clibs_arena_alloc(Clibs_Arena *arena, size_t size);
void clibs_arena_reset(Clibs_Arena *arena);
void clibs_arena_destroy(Clibs_Arena *arena);

#endif // CLIBS_ARENA

#ifdef CLIBS_ARENA_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>

#define CEARCH_ARENA_ALIGNMENT (sizeof(void*))

Clibs_Arena *clibs_arena_create(size_t capacity) {
    Clibs_Arena *arena = malloc(sizeof(Clibs_Arena));

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

void *clibs_arena_alloc(Clibs_Arena *arena, size_t size) {
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

void clibs_arena_reset(Clibs_Arena *arena) {
    arena->offset = 0;
}

void clibs_arena_destroy(Clibs_Arena *arena) {
    if (arena) {
        free(arena->buffer);
        free(arena);
    }
}

#endif // CLIBS_ARENA_IMPLEMENTATION
