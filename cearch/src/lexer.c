#include "./lexer.h"
#include "./location.h"

#include <alloca.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define LOCATION_SNAPSHOTS_CAPACITY 256
#define MAX_DIGIT_LENGTH 32
#define MAX_FLOAT_POINT_LENGTH 32
#define MAX_SYMBOL_LENGTH 64

static Cearch_Location location_snapshots[LOCATION_SNAPSHOTS_CAPACITY] = {0};
static int location_snapshots_size;

static void save_location_snapshot(Cearch_Location location) {
    assert(location_snapshots_size < LOCATION_SNAPSHOTS_CAPACITY && "exceeded location snapshots capacity");

    location_snapshots[location_snapshots_size++] = location;
}

static Cearch_Location get_location_snapshot() {
    assert(location_snapshots_size > 0 && "empty location snapshots");

    return location_snapshots[--location_snapshots_size];
}

static Cearch_Location get_location_from_lexer(const Cearch_Lexer *lexer) {
    return (Cearch_Location){
        .line = lexer->line,
        .col_start = lexer->col,
        .col_end = lexer->col + (lexer->cursor - lexer->bot)
    };
}

static void throw_error_message(Cearch_Location location, const char *fmt, ...) {
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

const char *cearch_token_kind_name(Cearch_Token_Kind kind) {
    switch (kind) {
        case CT_NIL: return "nil";
        case CT_BOOL: return "bool";
        case CT_STR: return "str";
        case CT_INT: return "int";
        case CT_FLOAT: return "float";

        case CT_LSQUARE: return "[";
        case CT_RSQUARE: return "]";
        case CT_LPAREN: return "(";
        case CT_RPAREN: return ")";
        case CT_LT: return "<";
        case CT_GT: return ">";
        case CT_LTE: return "<=";
        case CT_GTE: return ">=";
        case CT_EQ: return "=";
        case CT_NEQ: return "!=";
        case CT_NOT: return "!";

        case CT_OR: return "or";
        case CT_AND: return "and";

        case CT_SYM: return "sym";

        default: assert(0 && "cearch_token_kind_name: missing Cearch_Token_Kind");
    }
}

static Cearch_Token_Kind symbol_to_kind(Cearch_String symbol) {
    if (strncmp("nil", symbol.value, symbol.size) == 0) return CT_NIL;
    if (strncmp("true", symbol.value, symbol.size) == 0) return CT_BOOL;
    if (strncmp("false", symbol.value, symbol.size) == 0) return CT_BOOL;
    if (strncmp("and", symbol.value, symbol.size) == 0) return CT_AND;
    if (strncmp("or", symbol.value, symbol.size) == 0) return CT_OR;

    return CT_SYM;
}

static inline char chr(const Cearch_Lexer *const lexer) {
    return lexer->cursor < lexer->content_size ? lexer->content[lexer->cursor] : '\0';
}

static inline void advance_cursor(Cearch_Lexer *lexer) {
    if (lexer->content[lexer->cursor] == '\n') {
        lexer->col = 1;
        lexer->line++;
    } else {
        lexer->col++;
    }

    if (lexer->cursor < lexer->content_size) lexer->cursor++;
}

static inline void sync_bot(Cearch_Lexer *lexer) {
    lexer->bot = lexer->cursor;
}

static inline bool is_empty(const Cearch_Lexer * const lexer) {
    return chr(lexer) == '\0';
}

static inline bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline bool is_symbol(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static inline void ltrim_whitespaces(Cearch_Lexer *lexer) {
    while (!is_empty(lexer) && (chr(lexer) == '\n' || chr(lexer) == '\t' || chr(lexer) == ' ')) advance_cursor(lexer);
}

static inline void append_token(Cearch_Lexer *lexer, Cearch_Token *token) {
    if (lexer->head == NULL) {
        lexer->head = lexer->tail = token;
    } else {
        lexer->tail = lexer->tail->next = token;
    }
}

static void lex_digit(Cearch_Lexer *lexer) {
    while (!is_empty(lexer) && is_digit(chr(lexer))) advance_cursor(lexer);

    bool is_float = false;

    int left_digit_length = lexer->cursor - lexer->bot;

    if (chr(lexer) == '.') {
        is_float = true;

        advance_cursor(lexer);

        while (!is_empty(lexer) && is_digit(chr(lexer))) advance_cursor(lexer);
    }

    int digit_length = lexer->cursor - lexer->bot;

    if (left_digit_length > MAX_DIGIT_LENGTH)
        throw_error_message(get_location_snapshot(), "your digit ('%.*s') overflew the maximum length of %d digits", digit_length, lexer->content + lexer->bot, MAX_DIGIT_LENGTH);

    if (digit_length - left_digit_length - 1 > MAX_FLOAT_POINT_LENGTH)
        throw_error_message(get_location_snapshot(), "your digit ('%.*s') overflew the maximum float point length of %d digits", digit_length, lexer->content + lexer->bot, MAX_FLOAT_POINT_LENGTH);

    char *digit = alloca(digit_length + 1);

    memcpy(digit, lexer->content + lexer->bot, digit_length);

    digit[digit_length] = '\0';

    char *endptr;
    Cearch_Token *token = malloc(sizeof(Cearch_Token));

    if (is_float) {
        double value = strtod(digit, &endptr);

        if (digit == endptr) {
            throw_error_message(get_location_snapshot(), "could not parse '%s' as float", digit);
        }

        *token = (Cearch_Token){
            .content = (Cearch_String){
                .value = lexer->content + lexer->bot,
                .size = digit_length,
            },
            .kind = CT_FLOAT,
            .as_float = value,
            .location = get_location_snapshot(),
            .next = NULL
        };
    } else {
        long value = strtol(digit, &endptr, 10);

        if (digit == endptr) {
            throw_error_message(get_location_snapshot(), "could not parse '%s' as int", digit);
        }

        *token = (Cearch_Token){
            .content = (Cearch_String){
                .value = lexer->content + lexer->bot,
                .size = digit_length,
            },
            .kind = CT_INT,
            .as_int = value,
            .location = get_location_snapshot(),
            .next = NULL
        };
    }

    append_token(lexer, token);
}

static void lex_symbol(Cearch_Lexer *lexer) {
    int symbol_size = 0;

    while (!is_empty(lexer) && is_symbol(chr(lexer))) {
        symbol_size++;

        if (symbol_size > MAX_SYMBOL_LENGTH)
            throw_error_message(
                get_location_snapshot(),
                "your symbol (%.*s...) exceeded the max length of %d characters",
                MAX_SYMBOL_LENGTH,
                lexer->content + lexer->bot,
                MAX_SYMBOL_LENGTH
            );

        advance_cursor(lexer);
    }

    Cearch_String symbol = {
        .size = symbol_size,
        .value = lexer->content + lexer->bot
    };

    Cearch_Token_Kind kind = symbol_to_kind(symbol);

    Cearch_Token *token = malloc(sizeof(Cearch_Token));

    *token = (Cearch_Token){
        .kind = kind,
        .content = symbol,
        .location = get_location_snapshot(),
        .next = NULL
    };

    if (kind == CT_BOOL) {
        if (strncmp("true", symbol.value, symbol.size) == 0) token->as_bool = true;
        else if (strncmp("false", symbol.value, symbol.size) == 0) token->as_bool = false;
        else throw_error_message(token->location, "invalid boolean (%.*s)\n", symbol.size, symbol.value);
    } else if (kind != CT_NIL) {
        token->as_str = symbol;
    }

    append_token(lexer, token);
}

Cearch_Token *cearch_lex(Cearch_Lexer *lexer) {
    if (lexer->line == 0) lexer->line++;
    if (lexer->col == 0) lexer->col++;

    while (!is_empty(lexer)) {
        ltrim_whitespaces(lexer);

        if (is_empty(lexer)) break;

        sync_bot(lexer);

        save_location_snapshot(get_location_from_lexer(lexer));

        switch (chr(lexer)) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                lex_digit(lexer);
                break;
            default:
                if (is_symbol(chr(lexer))) {
                    lex_symbol(lexer);
                    break;
                }

                throw_error_message(get_location_snapshot(), "unrecognized character '%c'", chr(lexer));
                break;
        }
    }

    return lexer->head;
}
