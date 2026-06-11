#ifndef _CEARCH_PARSER_H_
#define _CEARCH_PARSER_H_

#include "./location.h"
#include "./string.h"
#include "./lexer.h"
#include "./arena.h"
#include "./types.h"

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
} Cearch_Ast_Expr_Kind;

typedef struct Cearch_Ast_Node Cearch_Ast_Node;

struct Cearch_Ast_Node {
    // this is the type of the expression.
    // It can be a method call, can be a binary expression
    // an unary, etc.
    Cearch_Ast_Expr_Kind kind;
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
    Cearch_Data_Type *dtype;

    Cearch_Location location;

    // reference to the text itself of the node
    Cearch_String raw_string;

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

const char *cearch_parser_node_type_name(Cearch_Ast_Expr_Kind type);
Cearch_Parser *cearch_create_parser(Cearch_Token *tokens_head);
Cearch_Ast_Node *cearch_parse_expression(Cearch_Parser *parser);
void cearch_free_parser(Cearch_Parser *parser);

#endif // _CEARCH_PARSER_H_
