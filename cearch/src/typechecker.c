#include "./typechecker.h"
#include "./location.h"
#include "ht.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static void display_error_and_exit(Cearch_Ast_Node *node, const char *message, const char *description, ...) {
    va_list args;
    va_start(args, description);

    args->fp_offset = 0;
    int padding = 4;

    int line = node->location.line;
    int start = node->location.col_start;
    int end = node->location.col_end;

    fprintf(stderr, "error(type inference) %d:%d: %s\n", line, start, message);
    fprintf(stderr, "\n");
    // TODO: truncate the text to the chunk that is relevant
    fprintf(stderr, "%*.s'%.*s'\n", padding, "", node->raw_string.size, node->raw_string.value);

    if (start != -1 && end != -1) {
        if (start == end) {
            fprintf(stderr, "%*.s^\n", padding + start, "");
        } else if (end > start) {
            fprintf(stderr, "%*.s", padding + start, "");
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

static Cearch_Data_Type *create_data_type(Cearch_Data_Type_Kind kind, bool nullable) {
    Cearch_Data_Type *dtype = malloc(sizeof(Cearch_Data_Type));

    dtype->kind = kind;
    dtype->nullable = nullable;
    dtype->inner = NULL;

    return dtype;
}

void infer_array_data_type(Cearch_Ast_Node *ast, Ht *variable_types) {
    if (ast->as_array.elements_length == 0) {
        display_error_and_exit(ast, "could not infer data type of expression", "it's not possible to infer the data type of an empty array");
    }

    cearch_data_type_infer(ast->as_array.elements[0], variable_types);

    Cearch_Data_Type *last_array_dtype = ast->as_array.elements[0]->dtype;

    for (int i = 1; i < ast->as_array.elements_length; i++) {
        cearch_data_type_infer(ast->as_array.elements[i], variable_types);

        if (!cearch_types_are_identical(ast->as_array.elements[i]->dtype, last_array_dtype)) {
            display_error_and_exit(ast, "you cannot have mixed arrays", "%*.s", ast->raw_string.size, ast->raw_string.value);
        }
    }

    Cearch_Data_Type *dtype = create_data_type(CDTK_ARRAY, false);

    dtype->inner = last_array_dtype;

    ast->dtype = dtype;
}

void cearch_data_type_infer(Cearch_Ast_Node *ast, Ht *variable_types) {
    switch (ast->kind) {
        case ANT_NIL:
            ast->dtype = create_data_type(CDTK_NIL, false);
            break;
        case ANT_INT:
            ast->dtype = create_data_type(CDTK_INT, false);
            break;
        case ANT_FLOAT:
            ast->dtype = create_data_type(CDTK_FLOAT, false);
            break;
        case ANT_STR:
            ast->dtype = create_data_type(CDTK_STR, false);
            break;
        case ANT_BOOL:
            ast->dtype = create_data_type(CDTK_BOOL, false);
            break;
        case ANT_ARRAY: 
            infer_array_data_type(ast, variable_types);
            break;
        case ANT_IDENTIFIER: {
            Cearch_Data_Type *dtype = ht_find(variable_types, ast->as_identifier.value);

            if (dtype == NULL)
                display_error_and_exit(ast, "undefined variable", "variable '%.*s' does not exists", ast->as_identifier.size, ast->as_identifier.value);

            ast->dtype = dtype;
        } break;
        case ANT_UNARY:
            break;
        case ANT_BINARY:
            break;            
        case ANT_METHOD_CALL:
            break;
    }
}
