#ifndef _CEARCH_PARSER_H_
#define _CEARCH_PARSER_H_

#include "./location.h"
#include "./string.h"
#include "./lexer.h"
#include "./arena.h"

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
} Cearch_Ast_Node_Type;

typedef struct Cearch_Ast_Node Cearch_Ast_Node;

struct Cearch_Ast_Node {
    Cearch_Ast_Node_Type type;

    Cearch_Location location;

    union {
        int           as_int;
        double        as_float;
        bool          as_bool;
        Cearch_String as_str;
        Cearch_String as_identifier;

        struct {
            Cearch_Token_Kind op;
            Cearch_Ast_Node*  operand;
        } as_unary;

        struct {
            Cearch_Token_Kind op;

            Cearch_Ast_Node* left;
            Cearch_Ast_Node* right;
        } as_binary;

        struct {
            // TODO: specify the return type?
            Cearch_Ast_Node* self;
            Cearch_String    method_name;

            Cearch_Ast_Node** arguments;
            int               arguments_length;
        } as_method_call;

        struct {
            Cearch_Ast_Node** elements;
            int               elements_length;
        } as_array;
    };
};

typedef struct {
    Cearch_Ast_Node* ast;
    Cearch_Token*    tokens_head;
    Clibs_Arena*    ast_arena;
    Clibs_Arena*    strs_arena;

    Cearch_Token *last_successfull_parsed_token;
} Cearch_Parser;

Cearch_Parser *cearch_create_parser(Cearch_Token *tokens_head);
Cearch_Ast_Node *cearch_parse_expression(Cearch_Parser *parser);
void cearch_free_parser(Cearch_Parser *parser);

#endif // _CEARCH_PARSER_H_
