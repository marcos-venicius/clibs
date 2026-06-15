#ifndef _cearch_amalg_h_
#define _cearch_amalg_h_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define __cearch_utilities_arena_alignment (sizeof(void*))
#define __cearch_lexer_location_snapshots_capacity 256
#define __cearch_lexer_max_digit_length 32
#define __cearch_lexer_max_float_point_length 32
#define __cearch_lexer_max_symbol_length 64
#define __cearch_lexer_max_string_length (1024 * 5)
#define __cearch_lexer_tokens_arena_capacity (sizeof(cearch_token_t) * 1024)
#define __cearch_lexer_strs_arena_capacity (32 * 1024)

#ifndef __cearch_arena

typedef struct {
    int line, col_start, col_end;
} cearch_location_t;

typedef struct {
    const char *value;
    int size;
} cearch_string_t;

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
} cearch_arena_t;

static cearch_arena_t *cearch_arena_create(size_t capacity) {
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

static void *cearch_arena_alloc(cearch_arena_t *arena, size_t size) {
    size_t padding = 0;
    uintptr_t current_ptr = (uintptr_t)(arena->buffer + arena->offset);
    uintptr_t remainder = current_ptr % __cearch_utilities_arena_alignment;

    if (remainder != 0) {
        padding = __cearch_utilities_arena_alignment - remainder;
    }

    if (arena->offset + padding + size > arena->capacity) {
        fprintf(stderr, "[CEARCH_ARENA_ERROR]: Arena ran out of memory\n");
        exit(1);
    }

    void *allocated_ptr = arena->buffer + arena->offset + padding;

    arena->offset += padding + size;

    return allocated_ptr;
}

static inline void clibs_arena_reset(cearch_arena_t *arena) {
    arena->offset = 0;
}

static inline void cearch_arena_destroy(cearch_arena_t *arena) {
    if (arena) {
        free(arena->buffer);
        free(arena);
    }
}

#endif // __cearch_arena

#ifndef __cearch_utilities

static bool cmp_const_sized_str(const char *conzt, const char *sized, int length) {
    int conzt_size = strlen(conzt);

    if (length != conzt_size) return false;

    return strncmp(conzt, sized, length) == 0;
}

#endif // __cearch_utilities

#ifndef __cearch_ht

#define __cearch_ht_size 1019

typedef struct ht_node_t ht_node_t;

struct ht_node_t {
    struct {
        char *key;
        int length;
    } name;
    void *data;
    ht_node_t *next;
};

typedef struct {
    int length;
    int data_size;
    ht_node_t *nodes[__cearch_ht_size];
} ht_t;

typedef struct {
    ht_t      *ht;  // hash table
    ht_node_t *it;  // current node on iteration
    int        idx; // current nodes array position on iteration
} ht_iterator_t;

static int ht_hash_key(const char *key) {
    const unsigned char *str = (const unsigned char*)key;

    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) hash = ((hash << 5) + hash) + c;

    return hash % __cearch_ht_size;
}

static ht_node_t *ht_alloc_node(ht_t *ht, const char *key, int key_length, void *data) {
    ht_node_t *node = malloc(sizeof(ht_node_t));

    // I'm using strdup (which includes the null-byte) because
    // we may want provide a loop for the client with all keys and values.
    // I don't want to mess up and let a segfault happen just because
    // the called is not concerned about this.
    node->name.key = strdup(key);
    node->name.length = key_length;
    node->next = NULL;
    node->data = malloc(ht->data_size);

    memcpy(node->data, data, ht->data_size);

    return node;
}

static inline void ht_free_node(ht_node_t *node) {
    free(node->data);
    free(node->name.key);
    free(node);
}

ht_t *ht_init(int data_size) {
    ht_t *ht = calloc(1, sizeof(ht_t));

    assert(data_size > 0 && "'data_size' should be greater than zero");

    ht->data_size = data_size;

    return ht;
}

void ht_add(ht_t *ht, const char *key, void *data) {
    // TODO: we're not rehashing the keys,
    //       we may do it in the future, but for the current
    //       purpose of this DS we don't need it.
    int index = ht_hash_key(key);
    int key_length = strlen(key);

    if (ht->nodes[index] == NULL) {
        ht->nodes[index] = ht_alloc_node(ht, key, key_length, data);
    } else {
        ht_node_t *slow = NULL;
        ht_node_t *fast = ht->nodes[index];

        while (fast != NULL) {
            // exact same key
            if (fast->name.length == key_length && (memcmp(fast->name.key, key, key_length) == 0)) {
                // substituted node with exact same key
                memcpy(fast->data, data, ht->data_size);
                return;
            }

            slow = fast;
            fast = fast->next;
        }

        // added new node at the end
        slow->next = ht_alloc_node(ht, key, key_length, data);
    }
}

void *ht_find(ht_t *ht, const char *key) {
    int index = ht_hash_key(key);
    int key_length = strlen(key);

    ht_node_t *curr = ht->nodes[index];

    while (curr != NULL) {
        if (curr->name.length == key_length && (memcmp(curr->name.key, key, key_length) == 0))
            return curr->data;

        curr = curr->next;
    }

    return NULL;
}

void ht_free(ht_t *ht) {
    // TODO: use arena?
    for (int i = 0; i < __cearch_ht_size; i++) {
        ht_node_t *head = ht->nodes[i];

        while (head != NULL) {
            ht_node_t *next = head->next;

            ht_free_node(head);

            head = next;
        }
    }

    free(ht);
}

// ITERATOR STUFF

ht_iterator_t ht_iterator(ht_t *ht) {
    return (ht_iterator_t){
        .ht = ht,
        .it = NULL,
        .idx = 0
    };
}

ht_node_t *ht_iterator_next(ht_iterator_t *it) {
    if (it == NULL || (it->idx >= __cearch_ht_size && it->it == NULL)) return NULL;

    if (it->it != NULL) {
        if (it->it->next != NULL)
            return it->it = it->it->next;
        else
            it->idx++;
    }

    while (it->idx < __cearch_ht_size) {
        if (it->ht->nodes[it->idx] != NULL) {
            it->it = it->ht->nodes[it->idx];

            return it->it;
        }

        it->idx++;
    }

    it->it = NULL;

    return NULL;
}

#endif // __cearch_ht

#ifndef __cearch_types

// the order of the fields matters
typedef enum {
    CDTK_NIL,
    CDTK_INT,
    CDTK_FLOAT,
    CDTK_STR,
    CDTK_BOOL,
    CDTK_ARRAY,
} cearch_data_type_enum_t;

typedef struct cearch_data_type_t cearch_data_type_t;

struct cearch_data_type_t {
    cearch_data_type_enum_t kind;
    bool nullable;

    // only used if kind is CDTK_ARRAY
    cearch_data_type_t *inner;
};

#endif // __cearch_types

#ifndef __cearch_lexer

typedef enum {
    // data types
    CTK_NIL = 0,
    CTK_BOOL,
    CTK_STR,
    CTK_INT,
    CTK_FLOAT,

    // operators
    CTK_LSQUARE,
    CTK_RSQUARE,
    CTK_LPAREN,
    CTK_RPAREN,
    CTK_COMMA,
    CTK_DOT,
    CTK_LT,
    CTK_GT,
    CTK_LTE,
    CTK_GTE,
    CTK_EQ,
    CTK_NEQ,
    CTK_NOT,

    // logical operators
    CTK_OR,
    CTK_AND,

    // symbols
    CTK_SYM,

    CTK_EOF
} cearch_token_kind_enum_t;

typedef struct cearch_token_t cearch_token_t;

struct cearch_token_t {
    cearch_location_t         location;
    cearch_token_kind_enum_t  kind;
    cearch_string_t           content; // actual string representation of the token

    cearch_token_t *next;

    union {
        cearch_string_t as_str;
        bool            as_bool;
        double          as_float;
        int             as_int;
    };
};

typedef struct {
    const char *content;
    int         content_size;

    int line, col, bot, cursor;

    cearch_token_t *head;
    cearch_token_t *tail;

    cearch_arena_t *tokens_arena;
    cearch_arena_t *strs_arena;
} cearch_lexer_t;

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

static const char *lexer_cearch_token_kind_enum_name(cearch_token_kind_enum_t kind) {
    switch (kind) {
        case CTK_NIL: return "nil";
        case CTK_BOOL: return "bool";
        case CTK_STR: return "str";
        case CTK_INT: return "int";
        case CTK_FLOAT: return "float";

        case CTK_LSQUARE: return "[";
        case CTK_RSQUARE: return "]";
        case CTK_LPAREN: return "(";
        case CTK_RPAREN: return ")";
        case CTK_COMMA: return ",";
        case CTK_DOT: return ".";
        case CTK_LT: return "<";
        case CTK_GT: return ">";
        case CTK_LTE: return "<=";
        case CTK_GTE: return ">=";
        case CTK_EQ: return "=";
        case CTK_NEQ: return "!=";
        case CTK_NOT: return "!";

        case CTK_OR: return "or";
        case CTK_AND: return "and";

        case CTK_SYM: return "sym";

        case CTK_EOF: return "eof";

        default: assert(0 && "cearch_token_kind_enum_name: missing cearch_token_kind_enum_t");
    }
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
    lexer_advance_cursor(lexer);

    int string_length = 0;

    while (!lexer_is_empty(lexer) && lexer_chr(lexer) != '\'') {
        string_length++;

        if (lexer_chr(lexer) == '\\') {
            switch (lexer_nchr(lexer)) {
                case '\'':
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

    if (lexer_chr(lexer) != '\'') {
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

cearch_lexer_t *lexer_create(const char *content, size_t content_size) {
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

cearch_token_t *lexer_run(cearch_lexer_t *lexer) {
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
            case '\'': lex_string(lexer); break;
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

void lexer_free(cearch_lexer_t *lexer) {
    cearch_arena_destroy(lexer->strs_arena);
    cearch_arena_destroy(lexer->tokens_arena);
    free(lexer);
}

#endif // __cearch_lexer

#ifndef __cearch_parser

#define __cearch_parser_max_function_arguments 32
#define __cearch_parser_max_array_length 256
#define __cearch_parser_ast_arena_capacity (sizeof(cearch_ast_node_t) * 512)
// 16 Kilobytes of memory should be enough to strings?
#define __cearch_parser_strs_arena_capacity (16 * 1024)

typedef enum {
    ANT_NIL,
    ANT_INT,
    ANT_FLOAT,
    ANT_STR,
    ANT_BOOL,
    ANT_ARRAY,
    ANT_IDENTIFIER,
    ANT_UNARY,
    ANT_BINARY,
    ANT_METHOD_CALL
} cearch_ast_expr_kind_t;

typedef struct cearch_ast_node_t cearch_ast_node_t;

struct cearch_ast_node_t {
    // this is the type of the expression.
    // It can be a method call, can be a binary expression
    // an unary, etc.
    cearch_ast_expr_kind_t kind;
    // this, otherwise, is the data type.
    // we have some obvious data types based on expression kind.
    //  nil, int, float, str and bool are correspondent to dtype.
    // they are even aligned (same values in both enums).
    //
    // We also have intrinsic types that based on the expression
    // it's impossible to have another type:
    //  unary and binary expression.
    // Both is required to result in a bool expression, so
    // the dtype is gonna be bool.
    //
    // But, when it comes for arrays, identifiers and method calls,
    // we have a more complex behavior.
    //
    // arrays:
    //  the root expression dtype is gonna be CDTK_ARRAY.
    //  but, the inner type will be based on the content of the array.
    //  Imagine this example: [[2, 4, 5]].
    //  The dtype structure is gonna be something like: { kind = CDTK_ARRAY, inner = { kind = CDTK_ARRAY, inner = { kind = CDTK_INT, inner = NULL } } }
    //  then, the final result is: array<array<int>>
    // identifiers:
    //  they are going to be inferred based on the user setting.
    //  So, the user should be able to, somehow, specify the type of all his identifiers.
    // method calls:
    //  the dtype is gonna hold the return type of the function.
    //  then, the function itself is gonna have a list of arguments that
    //  each one will contain your own type.
    //
    // By default, during parsing, the dtype is gonna be always null.
    // Only after type checking/inferring it's going to be populated.
    cearch_data_type_t *dtype;

    cearch_location_t location;

    // reference to the text itself of the node
    cearch_string_t raw_string;

    union {
        int             as_int;
        double          as_float;
        bool            as_bool;
        cearch_string_t as_str;
        cearch_string_t as_identifier;

        struct {
            cearch_token_kind_enum_t  op;
            cearch_ast_node_t        *operand;
        } as_unary;

        struct {
            cearch_token_kind_enum_t op;

            cearch_ast_node_t *left;
            cearch_ast_node_t *right;
        } as_binary;

        struct {
            cearch_ast_node_t *self;
            cearch_string_t    method_name;

            cearch_ast_node_t **arguments;
            int                 arguments_length;
        } as_method_call;

        struct {
            cearch_ast_node_t **elements;
            int                 elements_length;
        } as_array;
    };
};

typedef struct {
    cearch_ast_node_t  *ast;
    cearch_token_t     *tokens_head;
    cearch_arena_t     *ast_arena;
    cearch_arena_t     *strs_arena;
    cearch_token_t     *last_successfull_parsed_token;
} cearch_parser_t;

typedef enum {
    PREC_NONE,
    PREC_OR,
    PREC_AND,
    PREC_EQ,
    PREC_COMPARISON,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY,
} cearch_parser_precedence_t;

typedef cearch_ast_node_t *(*cearch_parser_prefix_fn_t)(cearch_parser_t *parser);
typedef cearch_ast_node_t *(*cearch_parser_infix_fn_t)(cearch_parser_t *parser, cearch_ast_node_t *left);

typedef struct {
    cearch_parser_prefix_fn_t prefix;
    cearch_parser_infix_fn_t infix;
    cearch_parser_precedence_t precedence;
} cearch_parse_rule_t;

// forward declarations
static cearch_ast_node_t *parse_unary(cearch_parser_t *parser);
static cearch_ast_node_t *parse_literal(cearch_parser_t *parser);
static cearch_ast_node_t *parse_identifier(cearch_parser_t *parser);
static cearch_ast_node_t *parse_array(cearch_parser_t *parser);
static cearch_ast_node_t *parse_expression(cearch_parser_t *parser, cearch_parser_precedence_t precedence);
static cearch_ast_node_t *parse_binary(cearch_parser_t *parser, cearch_ast_node_t *left);
static cearch_ast_node_t *parse_method(cearch_parser_t *parser, cearch_ast_node_t *left);
static cearch_ast_node_t *parse_group(cearch_parser_t *parser);

static cearch_parse_rule_t parsing_rules[] = {
    // literals and identifiers
    [CTK_INT]        = {parse_literal,       NULL,           PREC_NONE},
    [CTK_FLOAT]      = {parse_literal,       NULL,           PREC_NONE},
    [CTK_STR]        = {parse_literal,       NULL,           PREC_NONE},
    [CTK_BOOL]       = {parse_literal,       NULL,           PREC_NONE},
    [CTK_NIL]        = {parse_literal,       NULL,           PREC_NONE},
    [CTK_SYM]        = {parse_identifier,    NULL,           PREC_NONE},
    [CTK_LSQUARE]    = {parse_array,         NULL,           PREC_NONE},
    [CTK_LPAREN]     = {parse_group,         NULL,           PREC_NONE},

    // unary operators
    [CTK_NOT]        = {parse_unary,         NULL,           PREC_NONE},

    // infix operators
    [CTK_AND]        = {NULL,                parse_binary,   PREC_AND},
    [CTK_OR]         = {NULL,                parse_binary,   PREC_OR},
    [CTK_EQ]         = {NULL,                parse_binary,   PREC_EQ},
    [CTK_NEQ]        = {NULL,                parse_binary,   PREC_EQ},
    [CTK_GT]         = {NULL,                parse_binary,   PREC_COMPARISON},
    [CTK_LT]         = {NULL,                parse_binary,   PREC_COMPARISON},
    [CTK_GTE]        = {NULL,                parse_binary,   PREC_COMPARISON},
    [CTK_LTE]        = {NULL,                parse_binary,   PREC_COMPARISON},

    [CTK_DOT]        = {NULL,                parse_method,   PREC_CALL},
};

static inline cearch_parse_rule_t *parser_get_rule(cearch_token_kind_enum_t kind) {
    return &parsing_rules[kind];
}

static bool parser_is_empty(cearch_parser_t *parser) {
    return parser->tokens_head == NULL || parser->tokens_head->kind == CTK_EOF;
}

static inline cearch_token_t *parser_peek_token(cearch_parser_t *parser) {
    return parser->tokens_head;
}

static inline cearch_token_t *parser_consume_token(cearch_parser_t *parser) {
    if (parser_is_empty(parser)) return NULL;
    cearch_token_t *curr = parser->tokens_head;
    parser->tokens_head = parser->tokens_head->next;
    return curr;
}

static void parser_throw_error_message(cearch_location_t location, const char *fmt, ...) {
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

static cearch_ast_node_t *parse_literal(cearch_parser_t *parser) {
    cearch_token_t *token = parser_consume_token(parser);

    if (!token) return NULL;

    cearch_ast_node_t *node = cearch_arena_alloc(parser->ast_arena, sizeof(cearch_ast_node_t));
    node->location = token->location;
    node->raw_string = token->content;

    switch (token->kind) {
        case CTK_NIL:
            node->kind = ANT_NIL;
            break;
        case CTK_INT:
            node->kind = ANT_INT;
            node->as_int = token->as_int;
            break;
        case CTK_FLOAT:
            node->kind = ANT_FLOAT;
            node->as_float = token->as_float;
            break;
        case CTK_BOOL:
            node->kind = ANT_BOOL;
            node->as_bool = token->as_bool;
            break;
        case CTK_STR: {
            char *str = cearch_arena_alloc(parser->strs_arena, token->as_str.size + 1);

            memcpy(str, token->as_str.value, token->as_str.size);

            str[token->as_str.size] = '\0';

            node->kind = ANT_STR;
            node->as_str = (cearch_string_t){
                .size = token->as_str.size,
                .value = str
            };
        } break;
        default:
            parser_throw_error_message(
                token->location,
                "invalid syntax: expected nil, int, float, str or bool but got %s",
                lexer_cearch_token_kind_enum_name(token->kind)
            );
            break;
    }

    parser->last_successfull_parsed_token = token;

    return node;
}

static cearch_ast_node_t *parse_group(cearch_parser_t *parser) {
    // eat '('
    cearch_token_t *lparen = parser_consume_token(parser);

    parser->last_successfull_parsed_token = lparen;

    cearch_ast_node_t *node = parse_expression(parser, PREC_NONE);

    cearch_token_t *next = parser_peek_token(parser);

    if (next == NULL || next->kind != CTK_RPAREN) {
        parser_throw_error_message(lparen->location, "unterminated group, expected ')'");
    }

    // eat ')'
    parser->last_successfull_parsed_token = parser_consume_token(parser);

    return node;
}

static cearch_ast_node_t *parse_unary(cearch_parser_t *parser) {
    // eat '!'
    cearch_token_t *operator_token = parser_consume_token(parser);

    parser->last_successfull_parsed_token = operator_token;

    cearch_ast_node_t *operand = parse_expression(parser, PREC_UNARY);

    cearch_ast_node_t *node = cearch_arena_alloc(parser->ast_arena, sizeof(cearch_ast_node_t));
    node->kind = ANT_UNARY;
    node->location = operator_token->location;
    
    cearch_token_t *curr_token = parser_peek_token(parser);

    if (curr_token) {
        node->raw_string = (cearch_string_t){
            .value = operator_token->content.value,
            .size = curr_token->content.value - operator_token->content.value,
        };
    } else {
        node->raw_string = (cearch_string_t){
            .value = operator_token->content.value,
            .size = parser->last_successfull_parsed_token->content.value - operator_token->content.value + parser->last_successfull_parsed_token->content.size,
        };
    }

    node->as_unary.op = operator_token->kind;
    node->as_unary.operand = operand;

    return node;
}

static cearch_ast_node_t *parse_identifier(cearch_parser_t *parser) {
    cearch_token_t *token = parser_consume_token(parser);

    if (!token) return NULL;

    parser->last_successfull_parsed_token = token;

    cearch_ast_node_t *node = cearch_arena_alloc(parser->ast_arena, sizeof(cearch_ast_node_t));
    node->kind = ANT_IDENTIFIER;
    node->location = token->location;
    node->raw_string = token->content;

    char *str = cearch_arena_alloc(parser->strs_arena, token->as_str.size + 1);
    memcpy(str, token->as_str.value, token->as_str.size);
    str[token->as_str.size] = '\0';

    node->as_identifier = (cearch_string_t){
        .value = str,
        .size = token->as_str.size
    };

    return node;
}

static cearch_ast_node_t *parse_expression(cearch_parser_t *parser, cearch_parser_precedence_t precedence) {
    cearch_token_t *token = parser_peek_token(parser);

    if (!token) return NULL;

    cearch_parser_prefix_fn_t prefix_rule = parser_get_rule(token->kind)->prefix;

    if (prefix_rule == NULL) {
        parser_throw_error_message(token->location, "expected expression");
    }

    cearch_ast_node_t *left = prefix_rule(parser);

    parser->last_successfull_parsed_token = parser->tokens_head;

    while (parser_peek_token(parser) != NULL && precedence < parser_get_rule(parser_peek_token(parser)->kind)->precedence) {
        token = parser_peek_token(parser);

        cearch_parser_infix_fn_t infix_rule = parser_get_rule(token->kind)->infix;

        left = infix_rule(parser, left);
    }

    left->raw_string = (cearch_string_t){
        .value = token->content.value,
        .size = parser->last_successfull_parsed_token->content.value - token->content.value + parser->last_successfull_parsed_token->content.size,
    };

    return left;
}

static cearch_ast_node_t *parse_binary(cearch_parser_t *parser, cearch_ast_node_t *left) {
    cearch_token_t *operator = parser_consume_token(parser);

    cearch_parse_rule_t *rule = parser_get_rule(operator->kind);

    cearch_ast_node_t *right = parse_expression(parser, rule->precedence);

    cearch_ast_node_t *node = cearch_arena_alloc(parser->ast_arena, sizeof(cearch_ast_node_t));
    node->kind = ANT_BINARY;
    node->location = left->location;
    node->raw_string = (cearch_string_t){
        .value = operator->content.value,
        .size = parser->last_successfull_parsed_token->content.value - operator->content.value + parser->last_successfull_parsed_token->content.size,
    };

    node->as_binary.left = left;
    node->as_binary.right = right;
    node->as_binary.op = operator->kind;

    return node;
}

static cearch_ast_node_t *parse_array(cearch_parser_t *parser) {
    cearch_token_t *lbracket_token = parser_consume_token(parser);

    cearch_ast_node_t *node = cearch_arena_alloc(parser->ast_arena, sizeof(cearch_ast_node_t));

    node->kind = ANT_ARRAY;
    node->location = lbracket_token->location;
    node->as_array.elements = NULL;
    node->as_array.elements_length = 0;

    parser->last_successfull_parsed_token = parser_peek_token(parser);

    cearch_ast_node_t *temp_elements[__cearch_parser_max_array_length];
    int                temp_elements_length = 0;

    while (parser->tokens_head != NULL && parser->tokens_head->kind != CTK_RSQUARE) {
        if (temp_elements_length >= __cearch_parser_max_array_length) {
            parser_throw_error_message(lbracket_token->location, "array exceeds maximum length of %d", __cearch_parser_max_array_length);
        }

        temp_elements[temp_elements_length++] = parse_expression(parser, 0);

        if (parser->tokens_head->kind == CTK_COMMA) {
            parser->last_successfull_parsed_token = parser_consume_token(parser); // eat the ','
        } else if (parser->tokens_head->kind != CTK_RSQUARE) {
            parser_throw_error_message(
                parser->last_successfull_parsed_token->location,
                "expected ',' or ']' in array but got '%s'",
                lexer_cearch_token_kind_enum_name(parser->tokens_head->kind)
            );
        } else {
            parser->last_successfull_parsed_token = parser_peek_token(parser);
        }
    }

    if (parser->last_successfull_parsed_token != NULL && parser->last_successfull_parsed_token->kind == CTK_COMMA) {
        parser_throw_error_message(
            parser->last_successfull_parsed_token->location,
            "please, remove the trailing comma"
        );
    }

    if (parser->tokens_head == NULL || parser->tokens_head->kind != CTK_RSQUARE) {
        parser_throw_error_message(lbracket_token->location, "unterminated array, missing ']'");
    }

    cearch_token_t *rsquare = parser_consume_token(parser); // eat ']'

    node->raw_string = (cearch_string_t){
        .value = lbracket_token->content.value,
        .size = rsquare->content.value - rsquare->content.value + rsquare->content.size,
    };

    if (temp_elements_length > 0) {
        node->as_array.elements = cearch_arena_alloc(parser->ast_arena, temp_elements_length * sizeof(cearch_ast_node_t*));
        node->as_array.elements_length = temp_elements_length;
        memcpy(node->as_array.elements, temp_elements, temp_elements_length * sizeof(cearch_ast_node_t*));
    }
    
    return node;
}

static cearch_ast_node_t *parse_method(cearch_parser_t *parser, cearch_ast_node_t *left) {
    // eat '.'
    cearch_token_t *dot_token = parser_consume_token(parser);

    // eat the function name
    cearch_token_t *name_token = parser_consume_token(parser);

    if (!name_token || name_token->kind != CTK_SYM) {
        parser_throw_error_message(dot_token->location, "expected method name after '.'");
    }

    char *method_name = cearch_arena_alloc(parser->strs_arena, name_token->as_str.size + 1);
    memcpy(method_name, name_token->as_str.value, name_token->as_str.size);
    method_name[name_token->as_str.size] = '\0';

    cearch_ast_node_t *node = cearch_arena_alloc(parser->ast_arena, sizeof(cearch_ast_node_t));
    node->kind = ANT_METHOD_CALL;
    node->location = dot_token->location;
    node->as_method_call.method_name = (cearch_string_t){
        .value = method_name,
        .size = name_token->as_str.size
    };
    node->as_method_call.self = left;
    node->as_method_call.arguments = NULL;
    node->as_method_call.arguments_length = 0;

    cearch_token_t *last_method_call_expression_token = name_token;

    // zero-argument methods don't need parenthesis
    if (parser_peek_token(parser) != NULL && parser_peek_token(parser)->kind == CTK_LPAREN) {
        // eat '('
        parser_consume_token(parser);

        parser->last_successfull_parsed_token = parser->tokens_head;

        cearch_ast_node_t *temp_arguments[__cearch_parser_max_function_arguments];
        int                temp_arguments_length = 0;

        // parse until hit ')'
        while (parser_peek_token(parser) != NULL && parser_peek_token(parser)->kind != CTK_RPAREN) {
            if (temp_arguments_length >= __cearch_parser_max_function_arguments) {
                parser_throw_error_message(name_token->location, "function exceeds maximum of %d arguments", __cearch_parser_max_function_arguments);
            }

            temp_arguments[temp_arguments_length++] = parse_expression(parser, PREC_NONE);

            if (parser_peek_token(parser)->kind == CTK_COMMA) {
                // eat ','
                parser->last_successfull_parsed_token = parser_consume_token(parser);
            } else if (parser_peek_token(parser)->kind != CTK_RPAREN) {
                parser_throw_error_message(
                    parser->last_successfull_parsed_token->location,
                    "expected ',' or ')' in argument list but got '%s'",
                    lexer_cearch_token_kind_enum_name(parser_peek_token(parser)->kind)
                );
            } else {
                parser->last_successfull_parsed_token = parser_peek_token(parser);
            }
        }

        if (parser->last_successfull_parsed_token != NULL && parser->last_successfull_parsed_token->kind == CTK_COMMA) {
            parser_throw_error_message(
                parser->last_successfull_parsed_token->location,
                "please, remove the trailing comma"
            );
        }

        if (parser_peek_token(parser) == NULL || parser_peek_token(parser)->kind != CTK_RPAREN) {
            parser_throw_error_message(name_token->location, "unterminated argument list, missing ')'");
        }

        // eat ')'
        last_method_call_expression_token = parser_consume_token(parser);

        if (temp_arguments_length > 0) {
            node->as_method_call.arguments = cearch_arena_alloc(parser->ast_arena, temp_arguments_length * sizeof(cearch_ast_node_t*));
            node->as_method_call.arguments_length = temp_arguments_length;

            memcpy(node->as_method_call.arguments, temp_arguments, temp_arguments_length * sizeof(cearch_ast_node_t*));
        }
    }

    node->raw_string = (cearch_string_t){
        .value = left->raw_string.value,
        .size = last_method_call_expression_token->content.value - left->raw_string.value + last_method_call_expression_token->content.size,
    };

    return node;
}

static const char *parser_cearch_ast_expr_kind_name(cearch_ast_expr_kind_t type)
{
    switch (type) {
        case ANT_NIL: return "nil";
        case ANT_INT: return "int";
        case ANT_FLOAT: return "float";
        case ANT_STR: return "str";
        case ANT_BOOL: return "bool";
        case ANT_ARRAY: return "array";
        case ANT_IDENTIFIER: return "identifier";
        case ANT_UNARY: return "unary";
        case ANT_BINARY: return "binary";
        case ANT_METHOD_CALL: return "method_call";
        default: assert(0 && "parser_cearch_ast_expr_kind_name: missing cearch_ast_expr_kind_t handling"); break;
    }
}

cearch_parser_t *parser_create(cearch_token_t *tokens_head) {
    cearch_parser_t *parser = malloc(sizeof(cearch_parser_t));

    cearch_arena_t *strs_arena = cearch_arena_create(__cearch_parser_strs_arena_capacity);
    cearch_arena_t *ast_arena = cearch_arena_create(__cearch_parser_ast_arena_capacity);

    parser->ast_arena = ast_arena;
    parser->strs_arena = strs_arena;
    parser->tokens_head = tokens_head;

    return parser;
}

cearch_ast_node_t *parser_run(cearch_parser_t *parser) {
    cearch_ast_node_t *ast = parse_expression(parser, PREC_NONE);

    if (parser->tokens_head != NULL && parser->tokens_head->kind != CTK_EOF) {
        parser_throw_error_message(
            parser->last_successfull_parsed_token->location,
            "invalid syntax. expected an operator or EOF but got '%s'",
            lexer_cearch_token_kind_enum_name(parser->tokens_head->kind)
        );
    }

    return ast;
}

void cearch_free_parser(cearch_parser_t *parser) {
    cearch_arena_destroy(parser->ast_arena);
    cearch_arena_destroy(parser->strs_arena);
    free(parser);
}

#endif // __cearch_parser

#endif // _cearch_amalg_h_
