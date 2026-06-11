#include "./cearch.h"
#include "../lexer.h"
#include "../parser.h"
#include "../types.h"
#include "../arena.h"
#include "../typechecker.h"
#define HT_IMPLEMENTATION
#include "../ht.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct Cearch {
    Ht *variables;
    Clibs_Arena *dtypes_arena;
};

static void display_error_and_exit(const char *type_string, const char *func_name, const char *message, const char *description, int start, int end, ...) {
    va_list args;
    va_start(args, end);

    args->fp_offset = 0;
    int padding = 4;

    fprintf(stderr, "error(%s): %s\n", func_name, message);
    fprintf(stderr, "\n");
    // TODO: truncate the text to the chunk that is relevant
    fprintf(stderr, "%*.s'%s'\n", padding, "", type_string);

    if (start != -1 && end != -1) {
        if (start == end) {
            fprintf(stderr, "%*.s^\n", padding + start + 1, "");
        } else if (end > start) {
            fprintf(stderr, "%*.s", padding + start + 1, "");
            for (int i = start; i < end; i++) {
                fprintf(stderr, "~");
            }
            fprintf(stderr, "\n");
        }
    }

    if (description != NULL) {
        fprintf(stderr, "\n");
        fprintf(stderr, "%*.s", padding, "");
        vfprintf(stderr, description, args);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");

    va_end(args);

    exit(1);
}

Cearch *cearch_init(void) {
    Cearch *cearch = calloc(1, sizeof(Cearch));

    cearch->variables = ht_init(sizeof(Cearch_Data_Type));
    cearch->dtypes_arena = clibs_arena_create(sizeof(Cearch_Data_Type) * 512);

    return cearch;
}

void cearch_define_variable(Cearch *cearch, const char *variable_name, const char *variable_type) {
    Cearch_Data_Type *type = ht_find(cearch->variables, variable_name);

    if (type != NULL) {
        display_error_and_exit(
            variable_name,
            "cearch_define_variable",
            "you cannot define a variable twice",
            NULL,
            0,
            strlen(variable_name)
        );
    }

    // cearch_parse_data_type are going to exit in case it fails
    Cearch_Data_Type *dtype = cearch_parse_data_type(cearch->dtypes_arena, "cearch_define_variable", variable_type);

    ht_add(cearch->variables, variable_name, dtype);
}

void cearch_compile(Cearch *cearch, const char *expression) {
    Cearch_Lexer *lexer = cearch_create_lexer(expression, strlen(expression));

    Cearch_Token *tokens_head = cearch_lex(lexer);

    Cearch_Parser *parser = cearch_create_parser(tokens_head);

    Cearch_Ast_Node *ast = cearch_parse_expression(parser);

    cearch_data_type_infer(ast, (void *)&cearch->variables);
}

void cearch_debug_variables(Cearch *cearch) {
    Ht_Iterator it = ht_iterator(cearch->variables);
    Ht_Node* node;

    while ((node = ht_iterator_next(&it))) {
        printf("%.*s: ", node->name.length, node->name.key);
        cearch_printf_type((Cearch_Data_Type*)node->data);
        printf("\n");
    }
}

void cearch_free(Cearch *cearch) {
    clibs_arena_destroy(cearch->dtypes_arena);
    ht_free(cearch->variables);
    free(cearch);
}
