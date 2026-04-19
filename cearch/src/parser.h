#ifndef _CEARCH_PARSER_H_
#define _CEARCH_PARSER_H_

#include "./location.h"
#include "./string.h"
#include "./lexer.h"

#define MAX_FUNCTION_ARGUMENTS 32

typedef enum {
    ANT_INT,
    ANT_FLOAT,
    ANT_STR,
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

            Cearch_Ast_Node* arguments[32];
            int              arguments_length;
        } as_method_call;
    };
};

// TODO: Which algorithm should I use?
//       - Recursive Descent
//       - Pratt Parsing (I'm thinking to use this for the first time)
// 
// TODO: Setup Arena

#endif // _CEARCH_PARSER_H_
