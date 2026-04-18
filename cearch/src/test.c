#include <stdio.h>
#include <string.h>
#include "./lexer.h"

#define TEST_LEXER(q) do                                                        \
{                                                                               \
    printf("query: "q"\n");                                                     \
    Cearch_Lexer lexer = { .content = q, .content_size = strlen(q) };           \
    Cearch_Token *head = cearch_lex(&lexer);                                    \
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
    cearch_lexer_free(&lexer);                                                  \
} while(0);

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

    return 0;
}
