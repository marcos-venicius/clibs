#include "./parser.h"
#include "./arena.h"
#include "./lexer.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

typedef enum {
    PREC_NONE,
    PREC_OR,
    PREC_AND,
    PREC_EQ,
    PREC_COMPARISON,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY,
} Cearch_Parser_Precedence;

typedef Cearch_Ast_Node* (*Cearch_Parser_Prefix_Fn)(Cearch_Parser *parser);
typedef Cearch_Ast_Node* (*Cearch_Parser_Infix_Fn)(Cearch_Parser *parser, Cearch_Ast_Node *left);

typedef struct {
    Cearch_Parser_Prefix_Fn prefix;
    Cearch_Parser_Infix_Fn infix;
    Cearch_Parser_Precedence precedence;
} Cearch_Parse_Rule;

// 64 Kilobytes of memory should be enough to all kinds of expressions
#define PARSER_AST_ARENA_CAPACITY (64 * 1024)
// 16 Kilobytes of memory should be enough to strings?
#define PARSER_STRS_ARENA_CAPACITY (16 * 1024)

// forward declarations
static Cearch_Ast_Node *parse_unary(Cearch_Parser *parser);
static Cearch_Ast_Node *parse_literal(Cearch_Parser *parser);
static Cearch_Ast_Node *parse_identifier(Cearch_Parser *parser);
static Cearch_Ast_Node *parse_array(Cearch_Parser *parser);
static Cearch_Ast_Node *parse_expression(Cearch_Parser *parser, Cearch_Parser_Precedence precedence);
static Cearch_Ast_Node *parse_binary(Cearch_Parser *parser, Cearch_Ast_Node *left);
static Cearch_Ast_Node *parse_method(Cearch_Parser *parser, Cearch_Ast_Node *left);
static Cearch_Ast_Node *parse_group(Cearch_Parser *parser);

static Cearch_Parse_Rule parsing_rules[] = {
    // literals and identifiers
    [CT_INT]        = {parse_literal,       NULL,           PREC_NONE},
    [CT_FLOAT]      = {parse_literal,       NULL,           PREC_NONE},
    [CT_STR]        = {parse_literal,       NULL,           PREC_NONE},
    [CT_BOOL]       = {parse_literal,       NULL,           PREC_NONE},
    [CT_NIL]        = {parse_literal,       NULL,           PREC_NONE},
    [CT_SYM]        = {parse_identifier,    NULL,           PREC_NONE},
    [CT_LSQUARE]    = {parse_array,         NULL,           PREC_NONE},
    [CT_LPAREN]     = {parse_group,         NULL,           PREC_NONE},

    // unary operators
    [CT_NOT]        = {parse_unary,         NULL,           PREC_NONE},

    // infix operators
    [CT_AND]        = {NULL,                parse_binary,   PREC_AND},
    [CT_OR]         = {NULL,                parse_binary,   PREC_OR},
    [CT_EQ]         = {NULL,                parse_binary,   PREC_EQ},
    [CT_NEQ]        = {NULL,                parse_binary,   PREC_EQ},
    [CT_GT]         = {NULL,                parse_binary,   PREC_COMPARISON},
    [CT_LT]         = {NULL,                parse_binary,   PREC_COMPARISON},
    [CT_GTE]        = {NULL,                parse_binary,   PREC_COMPARISON},
    [CT_LTE]        = {NULL,                parse_binary,   PREC_COMPARISON},

    [CT_DOT]        = {NULL,                parse_method,   PREC_CALL},
};

static inline Cearch_Parse_Rule *get_rule(Cearch_Token_Kind kind) {
    return &parsing_rules[kind];
}

static bool is_empty(Cearch_Parser *parser) {
    return parser->tokens_head == NULL || parser->tokens_head->kind == CT_EOF;
}

static inline Cearch_Token *peek_token(Cearch_Parser *parser) {
    return parser->tokens_head;
}

static inline Cearch_Token *consume_token(Cearch_Parser *parser) {
    if (is_empty(parser)) return NULL;
    Cearch_Token *curr = parser->tokens_head;
    parser->tokens_head = parser->tokens_head->next;
    return curr;
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

static Cearch_Ast_Node *parse_literal(Cearch_Parser *parser) {
    Cearch_Token *token = consume_token(parser);

    if (!token) return NULL;

    Cearch_Ast_Node *node = arena_alloc(parser->ast_arena, sizeof(Cearch_Ast_Node));
    node->location = token->location;

    switch (token->kind) {
        case CT_NIL:
            node->type = ANT_NIL;
            break;
        case CT_INT:
            node->type = ANT_INT;
            node->as_int = token->as_int;
            break;
        case CT_FLOAT:
            node->type = ANT_FLOAT;
            node->as_float = token->as_float;
            break;
        case CT_BOOL:
            node->type = ANT_BOOL;
            node->as_bool = token->as_bool;
            break;
        case CT_STR: {
            char *str = arena_alloc(parser->strs_arena, token->as_str.size + 1);

            memcpy(str, token->as_str.value, token->as_str.size);

            str[token->as_str.size] = '\0';

            node->type = ANT_STR;
            node->as_str = (Cearch_String){
                .size = token->as_str.size,
                .value = str
            };
        } break;
        default:
            throw_error_message(
                token->location,
                "invalid syntax: expected nil, int, float, str or bool but got %s",
                cearch_token_kind_name(token->kind)
            );
            break;
    }

    parser->last_successfull_parsed_token = token;

    return node;
}

static Cearch_Ast_Node *parse_group(Cearch_Parser *parser) {
    // eat '('
    Cearch_Token *lparen = consume_token(parser);

    parser->last_successfull_parsed_token = lparen;

    Cearch_Ast_Node *node = parse_expression(parser, PREC_NONE);

    Cearch_Token *next = peek_token(parser);

    if (next == NULL || next->kind != CT_RPAREN) {
        throw_error_message(lparen->location, "unterminated group, expected ')'");
    }

    // eat ')'
    parser->last_successfull_parsed_token = consume_token(parser);

    return node;
}

static Cearch_Ast_Node *parse_unary(Cearch_Parser *parser) {
    // eat '!'
    Cearch_Token *operator_token = consume_token(parser);

    parser->last_successfull_parsed_token = operator_token;

    Cearch_Ast_Node *operand = parse_expression(parser, PREC_UNARY);

    Cearch_Ast_Node *node = arena_alloc(parser->ast_arena, sizeof(Cearch_Ast_Node));
    node->type = ANT_UNARY;
    node->location = operator_token->location;

    node->as_unary.op = operator_token->kind;
    node->as_unary.operand = operand;

    return node;
}

static Cearch_Ast_Node *parse_identifier(Cearch_Parser *parser) {
    Cearch_Token *token = consume_token(parser);

    if (!token) return NULL;

    parser->last_successfull_parsed_token = token;

    Cearch_Ast_Node *node = arena_alloc(parser->ast_arena, sizeof(Cearch_Ast_Node));
    node->type = ANT_IDENTIFIER;
    node->location = token->location;

    char *str = arena_alloc(parser->strs_arena, token->as_str.size + 1);
    memcpy(str, token->as_str.value, token->as_str.size);
    str[token->as_str.size] = '\0';

    node->as_identifier = (Cearch_String){
        .value = str,
        .size = token->as_str.size
    };

    return node;
}

static Cearch_Ast_Node *parse_expression(Cearch_Parser *parser, Cearch_Parser_Precedence precedence) {
    Cearch_Token *token = peek_token(parser);

    if (!token) return NULL;

    Cearch_Parser_Prefix_Fn prefix_rule = get_rule(token->kind)->prefix;

    if (prefix_rule == NULL) {
        throw_error_message(token->location, "expected expression");
    }

    Cearch_Ast_Node *left = prefix_rule(parser);

    parser->last_successfull_parsed_token = parser->tokens_head;

    while (peek_token(parser) != NULL && precedence < get_rule(peek_token(parser)->kind)->precedence) {
        token = peek_token(parser);

        Cearch_Parser_Infix_Fn infix_rule = get_rule(token->kind)->infix;

        left = infix_rule(parser, left);
    }

    return left;
}

static Cearch_Ast_Node *parse_binary(Cearch_Parser *parser, Cearch_Ast_Node *left) {
    Cearch_Token *operator = consume_token(parser);

    Cearch_Parse_Rule *rule = get_rule(operator->kind);

    Cearch_Ast_Node *right = parse_expression(parser, rule->precedence);

    Cearch_Ast_Node *node = arena_alloc(parser->ast_arena, sizeof(Cearch_Ast_Node));
    node->type = ANT_BINARY;
    node->location = left->location;

    node->as_binary.left = left;
    node->as_binary.right = right;
    node->as_binary.op = operator->kind;

    return node;
}

static Cearch_Ast_Node *parse_array(Cearch_Parser *parser) {
    Cearch_Token *lbracket_token = consume_token(parser);

    Cearch_Ast_Node *node = arena_alloc(parser->ast_arena, sizeof(Cearch_Ast_Node));

    node->type = ANT_ARRAY;
    node->location = lbracket_token->location;

    parser->last_successfull_parsed_token = peek_token(parser);

    while (parser->tokens_head != NULL && parser->tokens_head->kind != CT_RSQUARE) {
        if (node->as_array.elements_length >= MAX_ARRAY_LENGTH) {
            throw_error_message(lbracket_token->location, "array exceeds maximum length of %d", MAX_ARRAY_LENGTH);
        }

        node->as_array.elements[node->as_array.elements_length++] = parse_expression(parser, 0);

        if (parser->tokens_head->kind == CT_COMMA) {
            parser->last_successfull_parsed_token = consume_token(parser); // eat the ','
        } else if (parser->tokens_head->kind != CT_RSQUARE) {
            throw_error_message(
                parser->last_successfull_parsed_token->location,
                "expected ',' or ']' in array but got '%s'",
                cearch_token_kind_name(parser->tokens_head->kind)
            );
        } else {
            parser->last_successfull_parsed_token = peek_token(parser);
        }
    }

    if (parser->last_successfull_parsed_token != NULL && parser->last_successfull_parsed_token->kind == CT_COMMA) {
        throw_error_message(
            parser->last_successfull_parsed_token->location,
            "please, remove the trailing comma"
        );
    }

    if (parser->tokens_head == NULL || parser->tokens_head->kind != CT_RSQUARE) {
        throw_error_message(lbracket_token->location, "unterminated array, missing ']'");
    }

    consume_token(parser); // eat ']'
    
    return node;
}

static Cearch_Ast_Node *parse_method(Cearch_Parser *parser, Cearch_Ast_Node *left) {
    // eat '.'
    Cearch_Token *dot_token = consume_token(parser);

    // eat the function name
    Cearch_Token *name_token = consume_token(parser);

    if (!name_token || name_token->kind != CT_SYM) {
        throw_error_message(dot_token->location, "expected method name after '.'");
    }

    char *method_name = arena_alloc(parser->strs_arena, name_token->as_str.size + 1);
    memcpy(method_name, name_token->as_str.value, name_token->as_str.size);
    method_name[name_token->as_str.size] = '\0';

    Cearch_Ast_Node *node = arena_alloc(parser->ast_arena, sizeof(Cearch_Ast_Node));
    node->type = ANT_METHOD_CALL;
    node->location = dot_token->location;
    node->as_method_call.method_name = (Cearch_String){
        .value = method_name,
        .size = name_token->as_str.size
    };
    node->as_method_call.self = left;

    // zero-argument methods don't need parenthesis
    if (peek_token(parser) != NULL && peek_token(parser)->kind == CT_LPAREN) {
        // eat '('
        consume_token(parser);

        parser->last_successfull_parsed_token = parser->tokens_head;

        // parse until hit ')'
        while (peek_token(parser) != NULL && peek_token(parser)->kind != CT_RPAREN) {
            if (node->as_method_call.arguments_length >= MAX_FUNCTION_ARGUMENTS) {
                throw_error_message(name_token->location, "function exceeds maximum of %d arguments", MAX_FUNCTION_ARGUMENTS);
            }

            node->as_method_call.arguments[node->as_method_call.arguments_length++] = parse_expression(parser, PREC_NONE);

            if (peek_token(parser)->kind == CT_COMMA) {
                // eat ','
                parser->last_successfull_parsed_token = consume_token(parser);
            } else if (peek_token(parser)->kind != CT_RPAREN) {
                throw_error_message(
                    parser->last_successfull_parsed_token->location,
                    "expected ',' or ')' in argument list but got '%s'",
                    cearch_token_kind_name(peek_token(parser)->kind)
                );
            } else {
                parser->last_successfull_parsed_token = peek_token(parser);
            }
        }

        if (parser->last_successfull_parsed_token != NULL && parser->last_successfull_parsed_token->kind == CT_COMMA) {
            throw_error_message(
                parser->last_successfull_parsed_token->location,
                "please, remove the trailing comma"
            );
        }

        if (peek_token(parser) == NULL || peek_token(parser)->kind != CT_RPAREN) {
            throw_error_message(name_token->location, "unterminated argument list, missing ')'");
        }

        // eat ')'
        consume_token(parser);
    }

    return node;
}

Cearch_Parser *cearch_create_parser(Cearch_Token *tokens_head) {
    Cearch_Parser *parser = malloc(sizeof(Cearch_Parser));

    Cearch_Arena *strs_arena = arena_create(PARSER_STRS_ARENA_CAPACITY);
    Cearch_Arena *ast_arena = arena_create(PARSER_AST_ARENA_CAPACITY);

    parser->ast_arena = ast_arena;
    parser->strs_arena = strs_arena;
    parser->tokens_head = tokens_head;

    return parser;
}

Cearch_Ast_Node *cearch_parse_expression(Cearch_Parser *parser) {
    Cearch_Ast_Node *ast = parse_expression(parser, PREC_NONE);

    if (parser->tokens_head != NULL && parser->tokens_head->kind != CT_EOF) {
        throw_error_message(
            parser->last_successfull_parsed_token->location,
            "invalid syntax. expected an operator or EOF but got '%s'",
            cearch_token_kind_name(parser->tokens_head->kind)
        );
    }

    return ast;
}

void cearch_free_parser(Cearch_Parser *parser) {
    arena_destroy(parser->ast_arena);
    arena_destroy(parser->strs_arena);
    free(parser);
}
