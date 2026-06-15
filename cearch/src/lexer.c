#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "./lexer.h"

static cearch_location_t location_snapshots[__cearch_lexer_location_snapshots_capacity] = {0};
static int location_snapshots_size = 0;

static void lexer_save_location_snapshot(cearch_location_t location) {
    assert(location_snapshots_size < __cearch_lexer_location_snapshots_capacity && "exceeded location snapshots capacity");

    location_snapshots[location_snapshots_size++] = location;
}

static cearch_location_t lexer_pop_location_snapshot() {
    assert(location_snapshots_size > 0 && "empty location snapshots");

    return location_snapshots[--location_snapshots_size];
}

static cearch_location_t lexer_build_location_snapshot(const cearch_lexer_t *lexer) {
    return (cearch_location_t){
        .line = lexer->line,
        .col_start = lexer->col,
        .col_end = lexer->col + (lexer->cursor - lexer->bot)
    };
}

static void lexer_throw_error_message(cearch_location_t location, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (location.col_start == location.col_end) {
        fprintf(stderr, "error %d:%d: ", location.line, location.col_start);
    } else {
        fprintf(stderr, "error %d:%d-%d: ", location.line, location.col_start, location.col_end);
    }

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);

    exit(1);
}

static cearch_token_kind_enum_t cearch_string_as_token_kind_enum(cearch_string_t symbol) {
    if (cmp_const_sized_str("nil", symbol.value, symbol.size)) return CTK_NIL;
    if (cmp_const_sized_str("true", symbol.value, symbol.size)) return CTK_BOOL;
    if (cmp_const_sized_str("false", symbol.value, symbol.size)) return CTK_BOOL;
    if (cmp_const_sized_str("and", symbol.value, symbol.size)) return CTK_AND;
    if (cmp_const_sized_str("or", symbol.value, symbol.size)) return CTK_OR;

    return CTK_SYM;
}

static inline char lexer_chr(const cearch_lexer_t *const lexer) {
    return lexer->cursor < lexer->content_size ? lexer->content[lexer->cursor] : '\0';
}

static inline char lexer_nchr(const cearch_lexer_t *const lexer) {
    return lexer->cursor + 1 < lexer->content_size ? lexer->content[lexer->cursor + 1] : '\0';
}

static inline void lexer_advance_cursor(cearch_lexer_t *lexer) {
    if (lexer->content[lexer->cursor] == '\n') {
        lexer->col = 1;
        lexer->line++;
    } else {
        lexer->col++;
    }

    if (lexer->cursor < lexer->content_size) lexer->cursor++;
}

static inline void lexer_sync_bot(cearch_lexer_t *lexer) {
    lexer->bot = lexer->cursor;
}

static inline bool lexer_is_empty(const cearch_lexer_t *const lexer) {
    return lexer_chr(lexer) == '\0';
}

static inline bool lexer_is_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline bool lexer_is_symbol(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static inline void lexer_ltrim_whitespaces(cearch_lexer_t *lexer) {
    while (!lexer_is_empty(lexer) && (lexer_chr(lexer) == '\n' || lexer_chr(lexer) == '\t' || lexer_chr(lexer) == ' ')) lexer_advance_cursor(lexer);
}

static inline void lexer_append_token(cearch_lexer_t *lexer, cearch_token_t *token) {
    if (lexer->head == NULL) {
        lexer->head = lexer->tail = token;
    } else {
        lexer->tail = lexer->tail->next = token;
    }
}

static void lex_digit(cearch_lexer_t *lexer) {
    lexer_advance_cursor(lexer);

    while (!lexer_is_empty(lexer) && lexer_is_digit(lexer_chr(lexer))) lexer_advance_cursor(lexer);

    bool is_float = false;

    int left_digit_length = lexer->cursor - lexer->bot;

    if (lexer_chr(lexer) == '.') {
        is_float = true;

        lexer_advance_cursor(lexer);

        while (!lexer_is_empty(lexer) && lexer_is_digit(lexer_chr(lexer))) lexer_advance_cursor(lexer);
    }

    int digit_length = lexer->cursor - lexer->bot;

    if (left_digit_length > __cearch_lexer_max_digit_length)
        lexer_throw_error_message(
            lexer_pop_location_snapshot(),
            "your digit ('%.*s') overflew the maximum length of %d digits",
            digit_length,
            lexer->content + lexer->bot,
            __cearch_lexer_max_digit_length
        );

    if (digit_length - left_digit_length - 1 > __cearch_lexer_max_float_point_length)
        lexer_throw_error_message(
            lexer_pop_location_snapshot(),
            "your digit ('%.*s') overflew the maximum float point length of %d digits",
            digit_length,
            lexer->content + lexer->bot,
            __cearch_lexer_max_float_point_length
        );

    char *digit = alloca(digit_length + 1);

    memcpy(digit, lexer->content + lexer->bot, digit_length);

    digit[digit_length] = '\0';

    char *endptr;
    cearch_token_t *token = cearch_arena_alloc(lexer->tokens_arena, sizeof(cearch_token_t));

    if (is_float) {
        double value = strtod(digit, &endptr);

        if (digit == endptr) {
            lexer_throw_error_message(lexer_pop_location_snapshot(), "could not parse '%s' as float", digit);
        }

        *token = (cearch_token_t){
            .content = (cearch_string_t){
                .value = lexer->content + lexer->bot,
                .size = digit_length,
            },
            .kind = CTK_FLOAT,
            .as_float = value,
            .location = lexer_pop_location_snapshot(),
            .next = NULL
        };
    } else {
        long value = strtol(digit, &endptr, 10);

        if (digit == endptr) {
            lexer_throw_error_message(lexer_pop_location_snapshot(), "could not parse '%s' as int", digit);
        }

        *token = (cearch_token_t){
            .content = (cearch_string_t){
                .value = lexer->content + lexer->bot,
                .size = digit_length,
            },
            .kind = CTK_INT,
            .as_int = value,
            .location = lexer_pop_location_snapshot(),
            .next = NULL
        };
    }

    lexer_append_token(lexer, token);
}

static void lex_symbol(cearch_lexer_t *lexer) {
    int symbol_size = 0;

    while (!lexer_is_empty(lexer) && lexer_is_symbol(lexer_chr(lexer))) {
        symbol_size++;

        if (symbol_size > __cearch_lexer_max_symbol_length)
            lexer_throw_error_message(
                lexer_pop_location_snapshot(),
                "your symbol (%.*s...) exceeded the max length of %d characters",
                __cearch_lexer_max_symbol_length,
                lexer->content + lexer->bot,
                __cearch_lexer_max_symbol_length
            );

        lexer_advance_cursor(lexer);
    }

    cearch_string_t symbol = {
        .size = symbol_size,
        .value = lexer->content + lexer->bot
    };

    cearch_token_kind_enum_t kind = cearch_string_as_token_kind_enum(symbol);

    cearch_token_t *token = cearch_arena_alloc(lexer->tokens_arena, sizeof(cearch_token_t));

    *token = (cearch_token_t){
        .kind = kind,
        .content = symbol,
        .location = lexer_pop_location_snapshot(),
        .next = NULL
    };

    if (kind == CTK_BOOL) {
        if (cmp_const_sized_str("true", symbol.value, symbol.size)) token->as_bool = true;
        else if (cmp_const_sized_str("false", symbol.value, symbol.size)) token->as_bool = false;
        else lexer_throw_error_message(token->location, "invalid boolean (%.*s)\n", symbol.size, symbol.value);
    } else if (kind != CTK_NIL) {
        token->as_str = symbol;
    }

    lexer_append_token(lexer, token);
}

static void lex_string(cearch_lexer_t *lexer) {
    char quote = lexer_chr(lexer);

    lexer_advance_cursor(lexer);

    int string_length = 0;

    while (!lexer_is_empty(lexer) && lexer_chr(lexer) != quote) {
        string_length++;

        if (lexer_chr(lexer) == '\\') {
            switch (lexer_nchr(lexer)) {
                case '\'':
                case '"':
                case 'n':
                case 't':
                case '\\':
                    lexer_advance_cursor(lexer);
                    break;
                default:
                    lexer_throw_error_message(lexer_pop_location_snapshot(), "unrecognized escape sequence \\%c", lexer_nchr(lexer));
            }
        } else if (lexer_chr(lexer) == '\n') {
            lexer_throw_error_message(lexer_pop_location_snapshot(), "you cannot have a line break inside a string literal");
        }

        if (string_length > __cearch_lexer_max_string_length) {
            lexer_throw_error_message(lexer_pop_location_snapshot(), "string literal exceeded max length of %d characters", __cearch_lexer_max_string_length);
        }

        lexer_advance_cursor(lexer);
    }

    if (lexer_chr(lexer) != quote) {
        lexer_throw_error_message(
            lexer_pop_location_snapshot(),
            "unterminated string '%.*s'",
            lexer->cursor - lexer->bot,
            lexer->content + lexer->bot
        );
    }

    lexer_advance_cursor(lexer);

    cearch_token_t *token = cearch_arena_alloc(lexer->tokens_arena, sizeof(cearch_token_t));

    cearch_string_t content = {
        .value = lexer->content + lexer->bot + 1,
        .size = lexer->cursor - lexer->bot - 2
    };

    int i = 0;
    int str_index = 0;
    int str_size = lexer->cursor - lexer->bot - 2;

    char *str = cearch_arena_alloc(lexer->strs_arena, str_size + 1);

    assert(str != NULL && "could not allocate enough space for string");

    str[str_size] = '\0';

    while (i < str_size) {
        char next_char = i + 1 < str_size ? lexer->content[lexer->bot + i + 1 + 1] : '\0';
        char curr_char = lexer->content[lexer->bot + i + 1];

        if (curr_char == '\\') {
            switch (next_char) {
                case '\'':
                    str[str_index++] = '\'';
                    break;
                case '"':
                    str[str_index++] = '"';
                    break;
                case 'n':
                    str[str_index++] = '\n';
                    break;
                case 't':
                    str[str_index++] = '\t';
                    break;
                case '\\':
                    str[str_index++] = '\\';
                    break;
                default:
                    assert(0 && "this should never happen");
            }

            i++;
        } else {
            str[str_index++] = lexer->content[lexer->bot + i + 1];
        }

        i++;
    }

    *token = (cearch_token_t){
        .kind = CTK_STR,
        .content = content,
        .as_str = (cearch_string_t){
            .value = str,
            .size = str_size
        },
        .location = lexer_pop_location_snapshot(),
        .next = NULL
    };

    lexer_append_token(lexer, token);
}

static void lex_n(cearch_lexer_t *lexer, cearch_token_kind_enum_t kind, int n) {
    for (int i = 0; i < n; ++i) lexer_advance_cursor(lexer);

    cearch_token_t *token = cearch_arena_alloc(lexer->tokens_arena, sizeof(cearch_token_t));

    *token = (cearch_token_t){
        .kind = kind,
        .content = (cearch_string_t){
            .value = lexer->content + lexer->bot,
            .size = n
        },
        .location = lexer_pop_location_snapshot(),
        .next = NULL
    };

    lexer_append_token(lexer, token);
}

cearch_lexer_t *cearch_lexer_create(const char *content, size_t content_size) {
    cearch_lexer_t *lexer = malloc(sizeof(cearch_lexer_t));
    cearch_arena_t *tokens_arena = cearch_arena_create(__cearch_lexer_tokens_arena_capacity);
    cearch_arena_t *strs_arena = cearch_arena_create(__cearch_lexer_strs_arena_capacity);

    lexer->line = 1;
    lexer->col = 1;
    lexer->bot = 0;
    lexer->cursor = 0;
    lexer->content = content;
    lexer->content_size = content_size;
    lexer->head = NULL;
    lexer->tail = NULL;
    lexer->tokens_arena = tokens_arena;
    lexer->strs_arena = strs_arena;

    return lexer;
}

cearch_token_t *cearch_lexer_run(cearch_lexer_t *lexer) {
    while (true) {
        lexer_ltrim_whitespaces(lexer);

        lexer_sync_bot(lexer);

        lexer_save_location_snapshot(lexer_build_location_snapshot(lexer));

        if (lexer_is_empty(lexer)) break;

        switch (lexer_chr(lexer)) {
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': lex_digit(lexer); break;
            case '[': lex_n(lexer, CTK_LSQUARE, 1); break;
            case ']': lex_n(lexer, CTK_RSQUARE, 1); break;
            case '(': lex_n(lexer, CTK_LPAREN, 1); break;
            case ')': lex_n(lexer, CTK_RPAREN, 1); break;
            case ',': lex_n(lexer, CTK_COMMA, 1); break;
            case '.': lex_n(lexer, CTK_DOT, 1); break;
            case '=': lex_n(lexer, CTK_EQ, 1); break;
            case '\'':
            case '"':
                lex_string(lexer);
                break;
            case '!': {
                if (lexer_nchr(lexer) == '=') {
                    lex_n(lexer, CTK_NEQ, 2);
                } else {
                    lex_n(lexer, CTK_NOT, 1);
                }
            } break;
            case '>': {
                if (lexer_nchr(lexer) == '=') {
                    lex_n(lexer, CTK_GTE, 2);
                } else {
                    lex_n(lexer, CTK_GT, 1);
                }
            } break;
            case '<': {
                if (lexer_nchr(lexer) == '=') {
                    lex_n(lexer, CTK_LTE, 2);
                } else {
                    lex_n(lexer, CTK_LT, 1);
                }
            } break;
            default:
                if (lexer_is_symbol(lexer_chr(lexer))) {
                    lex_symbol(lexer);
                    break;
                }

                lexer_throw_error_message(lexer_pop_location_snapshot(), "unrecognized character '%c'", lexer_chr(lexer));
                break;
        }
    }

    cearch_token_t *eof = cearch_arena_alloc(lexer->tokens_arena, sizeof(cearch_token_t));
    *eof = (cearch_token_t){ .kind = CTK_EOF, .location = lexer_pop_location_snapshot() };

    lexer_append_token(lexer, eof);

    return lexer->head;
}

void cearch_lexer_free(cearch_lexer_t *lexer) {
    cearch_arena_destroy(lexer->strs_arena);
    cearch_arena_destroy(lexer->tokens_arena);
    free(lexer);
}
