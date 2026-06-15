#ifndef __cearch_parser_h_
#define __cearch_parser_h_

#include "./token.h"
#include "./libs/utils/utils.h"
#include "./libs/arena/arena.h"

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

#define __cearch_parser_max_function_arguments 32
#define __cearch_parser_max_array_length 256
#define __cearch_parser_ast_arena_capacity (sizeof(cearch_ast_node_t) * 512)
// 16 Kilobytes of memory should be enough to strings?
#define __cearch_parser_strs_arena_capacity (16 * 1024)

void cearch_parser_free(cearch_parser_t *parser);
cearch_ast_node_t *cearch_parser_run(cearch_parser_t *parser);
cearch_parser_t *cearch_parser_create(cearch_token_t *tokens_head);
const char *parser_cearch_ast_expr_kind_name(cearch_ast_expr_kind_t type);

#endif // __cearch_parser_h_
