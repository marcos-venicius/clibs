#ifndef __cearch_lexer_h_
#define __cearch_lexer_h_

#include "./token.h"
#include "./libs/utils/utils.h"
#include "./libs/arena/arena.h"

typedef struct {
    const char *content;
    int         content_size;

    int line, col, bot, cursor;

    cearch_token_t *head;
    cearch_token_t *tail;

    cearch_arena_t *tokens_arena;
    cearch_arena_t *strs_arena;
} cearch_lexer_t;

#define __cearch_lexer_location_snapshots_capacity 256
#define __cearch_lexer_max_digit_length 32
#define __cearch_lexer_max_float_point_length 32
#define __cearch_lexer_max_symbol_length 64
#define __cearch_lexer_max_string_length (1024 * 5)
#define __cearch_lexer_tokens_arena_capacity (sizeof(cearch_token_t) * 1024)
#define __cearch_lexer_strs_arena_capacity (32 * 1024)

cearch_lexer_t *cearch_lexer_create(const char *content, size_t content_size);
cearch_token_t *cearch_lexer_run(cearch_lexer_t *lexer);
void cearch_lexer_free(cearch_lexer_t *lexer);

#endif // __cearch_lexer_h_
