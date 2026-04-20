#include "./lexer.h"
#include "./parser.h"

#include <stdio.h>
#include <string.h>

#define TEST_LEXER(q) do                                                        \
{                                                                               \
    printf("query: "q"\n");                                                     \
    Cearch_Lexer *lexer = cearch_create_lexer(q, strlen(q));                     \
    Cearch_Token *head = cearch_lex(lexer);                                    \
    if (head == NULL) {                                                         \
        printf("  (null)\n");                                                   \
    } else {                                                                    \
        printf("  %s\n", cearch_token_kind_name(head->kind));                   \
        switch (head->kind) {                                                   \
            case CT_NIL: printf("  nil\n"); break;                              \
            case CT_BOOL: printf(head->as_bool ? "  true" : "  false"); break;  \
            case CT_INT: printf("  %d\n", head->as_int); break;                 \
            case CT_FLOAT: printf("  %lf\n", head->as_float); break;            \
            case CT_STR: printf("  %s\n", head->as_str.value); break;           \
            default:                                                            \
                printf("  %.*s\n", head->content.size, head->content.value);    \
                break;                                                          \
        }                                                                       \
    }                                                                           \
    printf("\n");                                                               \
    cearch_lexer_free(lexer);                                                  \
} while(0);

void cearch_print_ast(Cearch_Ast_Node *node, int depth) {
    if (!node) return;

    // 1. Print Indentation
    for (int i = 0; i < depth; i++) printf("  │ ");

    // 2. Handle Node Types
    switch (node->type) {
        case ANT_INT:
            printf("INT: %d\n", node->as_int);
            break;
        case ANT_FLOAT:
            printf("FLOAT: %f\n", node->as_float);
            break;
        case ANT_BOOL:
            printf("BOOL: %s\n", node->as_bool ? "true" : "false");
            break;
        case ANT_STR:
            printf("STR: \"%s\"\n", node->as_str.value);
            break;
        case ANT_IDENTIFIER:
            printf("ID: %s\n", node->as_identifier.value);
            break;

        case ANT_UNARY:
            printf("UNARY_OP: %s\n", cearch_token_kind_name(node->as_unary.op));
            cearch_print_ast(node->as_unary.operand, depth + 1);
            break;

        case ANT_BINARY:
            printf("BINARY_OP: %s\n", cearch_token_kind_name(node->as_binary.op));
            cearch_print_ast(node->as_binary.left, depth + 1);
            cearch_print_ast(node->as_binary.right, depth + 1);
            break;

        case ANT_METHOD_CALL:
            printf("METHOD_CALL: .%s\n", node->as_method_call.method_name.value);
            // Print 'self' (the object the method is called on)
            for (int i = 0; i <= depth; i++) printf("  │ ");
            printf("SELF:\n");
            cearch_print_ast(node->as_method_call.self, depth + 2);
            
            // Print arguments
            if (node->as_method_call.arguments_length > 0) {
                for (int i = 0; i <= depth; i++) printf("  │ ");
                printf("ARGS:\n");
                for (int i = 0; i < node->as_method_call.arguments_length; i++) {
                    cearch_print_ast(node->as_method_call.arguments[i], depth + 2);
                }
            }
            break;

        case ANT_ARRAY:
            printf("ARRAY: [%d elements]\n", node->as_array.elements_length);
            for (int i = 0; i < node->as_array.elements_length; i++) {
                cearch_print_ast(node->as_array.elements[i], depth + 1);
            }
            break;

        default:
            printf("UNKNOWN_NODE_TYPE\n");
            break;
    }
}

int main(void) {
    TEST_LEXER("");
    TEST_LEXER("1");
    TEST_LEXER("\n                    \n\n\n\n1\n\n     ");
    TEST_LEXER("1.");
    TEST_LEXER("1.34");
    TEST_LEXER("true");
    TEST_LEXER("false");
    TEST_LEXER("nil");
    TEST_LEXER("and");
    TEST_LEXER("or");
    TEST_LEXER("ltrim");
    TEST_LEXER("[");
    TEST_LEXER("]");
    TEST_LEXER("(");
    TEST_LEXER(")");
    TEST_LEXER(",");
    TEST_LEXER(".");
    TEST_LEXER("!");
    TEST_LEXER("=");
    TEST_LEXER("!=");
    TEST_LEXER(">");
    TEST_LEXER(">=");
    TEST_LEXER("<");
    TEST_LEXER("<=");
    TEST_LEXER("'|Hello \\\\ world\\'s!\\t|\\n  |'");
    TEST_LEXER("''");

    {
        printf("001\n");

        char *expression = "'  Hello. World  '.replace('.', ',').ltrim.rtrim().lower.debug";

        Cearch_Lexer *lexer = cearch_create_lexer(expression, strlen(expression));

        Cearch_Token *head = cearch_lex(lexer);

        Cearch_Parser *parser = cearch_create_parser(head);

        Cearch_Ast_Node *ast = cearch_parse_expression(parser);

        cearch_print_ast(ast, 0);

        cearch_free_parser(parser);
        cearch_lexer_free(lexer);
    }

    printf("\n\n");

    {
        printf("002\n");

        char *expression = "(status >= 200 and status < 300) or path.trim.lower = '/api/tracking'";

        Cearch_Lexer *lexer = cearch_create_lexer(expression, strlen(expression));

        Cearch_Token *head = cearch_lex(lexer);

        Cearch_Parser *parser = cearch_create_parser(head);

        Cearch_Ast_Node *ast = cearch_parse_expression(parser);

        cearch_print_ast(ast, 0);

        cearch_free_parser(parser);
        cearch_lexer_free(lexer);
    }

    printf("\n\n");

    {
        printf("003\n");

        char *expression = "(((!(false) or (!!true))))";

        Cearch_Lexer *lexer = cearch_create_lexer(expression, strlen(expression));

        Cearch_Token *head = cearch_lex(lexer);

        Cearch_Parser *parser = cearch_create_parser(head);

        Cearch_Ast_Node *ast = cearch_parse_expression(parser);

        cearch_print_ast(ast, 0);

        cearch_free_parser(parser);
        cearch_lexer_free(lexer);
    }

    printf("\n\n");

    {
        printf("004\n");

        char *expression = "![1, 2, 3, 5, 8, 13].contains(5) or [[1, 2, 4], [1, 2, 3], [0]].contains([0])";

        Cearch_Lexer *lexer = cearch_create_lexer(expression, strlen(expression));

        Cearch_Token *head = cearch_lex(lexer);

        Cearch_Parser *parser = cearch_create_parser(head);

        Cearch_Ast_Node *ast = cearch_parse_expression(parser);

        cearch_print_ast(ast, 0);

        cearch_free_parser(parser);
        cearch_lexer_free(lexer);
    }

    return 0;
}
