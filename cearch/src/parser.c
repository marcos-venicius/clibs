#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "./parser.h"

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
                cearch_token_kind_enum_name(token->kind)
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
                cearch_token_kind_enum_name(parser->tokens_head->kind)
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
                    cearch_token_kind_enum_name(parser_peek_token(parser)->kind)
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

const char *parser_cearch_ast_expr_kind_name(cearch_ast_expr_kind_t type)
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

cearch_parser_t *cearch_parser_create(cearch_token_t *tokens_head) {
    cearch_parser_t *parser = malloc(sizeof(cearch_parser_t));

    cearch_arena_t *strs_arena = cearch_arena_create(__cearch_parser_strs_arena_capacity);
    cearch_arena_t *ast_arena = cearch_arena_create(__cearch_parser_ast_arena_capacity);

    parser->ast_arena = ast_arena;
    parser->strs_arena = strs_arena;
    parser->tokens_head = tokens_head;

    return parser;
}

cearch_ast_node_t *cearch_parser_run(cearch_parser_t *parser) {
    cearch_ast_node_t *ast = parse_expression(parser, PREC_NONE);

    if (parser->tokens_head != NULL && parser->tokens_head->kind != CTK_EOF) {
        parser_throw_error_message(
            parser->last_successfull_parsed_token->location,
            "invalid syntax. expected an operator or EOF but got '%s'",
            cearch_token_kind_enum_name(parser->tokens_head->kind)
        );
    }

    return ast;
}

void cearch_parser_free(cearch_parser_t *parser) {
    cearch_arena_destroy(parser->ast_arena);
    cearch_arena_destroy(parser->strs_arena);
    free(parser);
}
