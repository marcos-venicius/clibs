#include "./lexer.h"
#include "./parser.h"

#include <stdio.h>
#include <string.h>

#define TEST_LEXER(q) do                                                        \
{                                                                               \
    printf("query: "q"\n");                                                     \
    Cearch_Lexer *lexer = cearch_create_lexer(q, strlen(q));                    \
    Cearch_Token *head = cearch_lex(lexer);                                     \
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
    cearch_lexer_free(lexer);                                                   \
} while(0);

#define TEST_PARSER(label, expression)                                              \
    do {                                                                            \
        printf("TEST_PARSER("label", \""expression"\"):\n");                        \
        Cearch_Lexer *lexer = cearch_create_lexer(expression, strlen(expression));  \
        Cearch_Token *head = cearch_lex(lexer);                                     \
        Cearch_Parser *parser = cearch_create_parser(head);                         \
        Cearch_Ast_Node *ast = cearch_parse_expression(parser);                     \
        printf("remounted ast: ");                                                  \
        print_ast_back_as_code(ast);                                                \
        printf("\n");                                                               \
        printf("ast graph:\n");                                                     \
        cearch_print_ast(ast, 1);                                             \
        cearch_free_parser(parser);                                                 \
        cearch_lexer_free(lexer);                                                   \
        printf("\n\n");                                                             \
    } while (0)

void cearch_print_ast(Cearch_Ast_Node *node, int depth) {
    if (!node) return;

    // 1. Print Indentation
    for (int i = 0; i < depth; i++) printf(" ");

    // 2. Handle Node Types
    switch (node->kind) {
        case ANT_INT:
            printf("int(%d)\n", node->as_int);
            break;
        case ANT_FLOAT:
            printf("float(%f)\n", node->as_float);
            break;
        case ANT_BOOL:
            printf("bool(%s)\n", node->as_bool ? "true" : "false");
            break;
        case ANT_STR:
            printf("str('%s')\n", node->as_str.value);
            break;
        case ANT_IDENTIFIER:
            printf("sym(%s)\n", node->as_identifier.value);
            break;

        case ANT_UNARY:
            printf("unary(%s):\n", cearch_token_kind_name(node->as_unary.op));
            cearch_print_ast(node->as_unary.operand, depth + 1);
            break;

        case ANT_BINARY:
            printf("binary(%s):\n", cearch_token_kind_name(node->as_binary.op));
            cearch_print_ast(node->as_binary.left, depth + 1);
            cearch_print_ast(node->as_binary.right, depth + 1);
            break;

        case ANT_METHOD_CALL:
            printf("call(.%s):\n", node->as_method_call.method_name.value);
            // Print 'self' (the object the method is called on)
            for (int i = 0; i <= depth; i++) printf(" ");
            printf("self:\n");
            cearch_print_ast(node->as_method_call.self, depth + 2);
            
            // Print arguments
            if (node->as_method_call.arguments_length > 0) {
                for (int i = 0; i <= depth; i++) printf(" ");
                printf("args: \n");
                for (int i = 0; i < node->as_method_call.arguments_length; i++) {
                    cearch_print_ast(node->as_method_call.arguments[i], depth + 2);
                }
            }
            break;

        case ANT_ARRAY:
            printf("array(len: %d):\n", node->as_array.elements_length);
            for (int i = 0; i < node->as_array.elements_length; i++) {
                cearch_print_ast(node->as_array.elements[i], depth + 1);
            }
            break;

        default:
            printf("UNKNOWN_NODE_TYPE\n");
            break;
    }
}

void print_ast_back_as_code(Cearch_Ast_Node *node) {
    switch (node->kind) {
        case ANT_NIL:
            printf("nil");
            break;
        case ANT_INT:
            printf("%d", node->as_int);
            break;
        case ANT_FLOAT:
            printf("%f", node->as_float);
            break;
        case ANT_BOOL:
            printf("%s", node->as_bool ? "true" : "false");
            break;
        case ANT_STR:
            printf("'%.*s'", node->as_str.size, node->as_str.value);
            break;
        case ANT_ARRAY:
            printf("[");
            for (int i = 0; i < node->as_array.elements_length; ++i) {
                if (i > 0) printf(", ");

                print_ast_back_as_code(node->as_array.elements[i]);
            }
            printf("]");
            break;
        case ANT_IDENTIFIER:
            printf("%.*s", node->as_identifier.size, node->as_identifier.value);
            break;
        case ANT_METHOD_CALL:
            print_ast_back_as_code(node->as_method_call.self);
            printf(".");
            printf("%.*s", node->as_method_call.method_name.size, node->as_method_call.method_name.value);

            if (node->as_method_call.arguments_length > 0) {
                printf("(");
                for (int i = 0; i < node->as_method_call.arguments_length; ++i) {
                    if (i > 0) printf(", ");

                    print_ast_back_as_code(node->as_method_call.arguments[i]);
                }
                printf(")");
            }
            break;
        case ANT_BINARY:
            if (node->as_binary.op == CT_OR || node->as_binary.op == CT_AND) printf("(");
            print_ast_back_as_code(node->as_binary.left);
            if (node->as_binary.op == CT_OR || node->as_binary.op == CT_AND) printf(")");

            printf(" %s ", cearch_token_kind_name(node->as_binary.op));

            if (node->as_binary.op == CT_OR || node->as_binary.op == CT_AND) printf("(");
            print_ast_back_as_code(node->as_binary.right);
            if (node->as_binary.op == CT_OR || node->as_binary.op == CT_AND) printf(")");
            break;
        case ANT_UNARY:
            printf("%s", cearch_token_kind_name(node->as_unary.op));
            printf("(");
            print_ast_back_as_code(node->as_unary.operand);
            printf(")");
            break;
        default:
            printf("%s", cearch_parser_node_type_name(node->kind));
            break;
    }
}

int main(int argc, char **argv) {
    if (argc > 2) {
        printf("usage: %s [query]\n", *argv);
        return 1;
    }

    if (argc == 2) {
        char *expression = *(argv + 1);

        Cearch_Lexer *lexer = cearch_create_lexer(expression, strlen(expression));
        Cearch_Token *head = cearch_lex(lexer);
        Cearch_Parser *parser = cearch_create_parser(head);
        Cearch_Ast_Node *ast = cearch_parse_expression(parser);
        printf("\nast back to code: ");
        print_ast_back_as_code(ast);
        printf("\n\nast tree:\n");
        cearch_print_ast(ast, 1);
        cearch_free_parser(parser);
        cearch_lexer_free(lexer);
        printf("\n");                                                               \

        return 0;
    }

    TEST_LEXER("");
    TEST_LEXER("1");
    TEST_LEXER("-1");
    TEST_LEXER("\n                    \n\n\n\n1\n\n     ");
    TEST_LEXER("1.");
    TEST_LEXER("-1.");
    TEST_LEXER("1.34");
    TEST_LEXER("-1.34");
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

    TEST_PARSER("001",  "'  Hello. World  '.replace('.', ',').ltrim.rtrim().lower.debug");
    TEST_PARSER("002",  "(status >= 200 and status < 300) or path.trim.lower = '/api/tracking'");
    TEST_PARSER("003", "(((!(false) or (!!true))))");
    TEST_PARSER("004", "![1, 2, 3, 5, 8, 13].contains(5) or [[1, 2, 4], [1, 2, 3], [0]].contains([0])");

    return 0;
}
