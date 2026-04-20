#ifndef _CEARCH_LEXER_H_
#define _CEARCH_LEXER_H_

#include "./string.h"
#include "./location.h"
#include "./arena.h"

#include <stdbool.h>

typedef enum {
    // data types
    CT_NIL = 0,
    CT_BOOL,
    CT_STR,
    CT_INT,
    CT_FLOAT,

    // operators
    CT_LSQUARE,
    CT_RSQUARE,
    CT_LPAREN,
    CT_RPAREN,
    CT_COMMA,
    CT_DOT,
    CT_LT,
    CT_GT,
    CT_LTE,
    CT_GTE,
    CT_EQ,
    CT_NEQ,
    CT_NOT,

    // logical operators
    CT_OR,
    CT_AND,

    // symbols
    CT_SYM,

    CT_EOF
} Cearch_Token_Kind;


typedef struct Cearch_Token Cearch_Token;

struct Cearch_Token {
    Cearch_Location    location;
    Cearch_Token_Kind  kind;
    Cearch_String      content; // actual string representation of the token

    Cearch_Token *next;

    union {
        Cearch_String as_str;
        bool          as_bool;
        double        as_float;
        int           as_int;
    };
};

typedef struct {
    char *content;
    int content_size;

    int line, col, bot, cursor;

    Cearch_Token *head;
    Cearch_Token *tail;

    Clibs_Arena *tokens_arena;
    Clibs_Arena *strs_arena;
} Cearch_Lexer;

Cearch_Lexer *cearch_create_lexer(char *content, size_t content_size);
Cearch_Lexer *cearch_create_lexer(char *content, size_t content_size);
Cearch_Token *cearch_lex(Cearch_Lexer *lexer);
const char *cearch_token_kind_name(Cearch_Token_Kind kind);
void cearch_lexer_free(Cearch_Lexer *lexer);

#endif // _CEARCH_LEXER_H_
