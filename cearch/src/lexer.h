#ifndef _CEARCH_LEXER_H_
#define _CEARCH_LEXER_H_

#include "./string.h"
#include "./location.h"

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
    CT_SYM
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
    const char *content;
    int content_size;

    int line, col, bot, cursor;

    Cearch_Token *head;
    Cearch_Token *tail;
} Cearch_Lexer;

Cearch_Token *cearch_lex(Cearch_Lexer *lexer);
const char *cearch_token_kind_name(Cearch_Token_Kind kind);

#endif // _CEARCH_LEXER_H_
