#include "./lexer.h"
#include "./location.h"

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#define LOCATION_SNAPSHOTS_CAPACITY 256

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
        .col_end = lexer->cursor
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

Cearch_Token *cearch_lex(Cearch_Lexer *lexer) {
    save_location_snapshot(get_location_from_lexer(lexer));

    throw_error_message(get_location_snapshot(), "uninitialized lexer");

    return lexer->head;
}
