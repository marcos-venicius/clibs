#include "./types.h"
#include "./arena.h"

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static inline bool is_type_name(char c) { return c >= 'a' && c <= 'z'; }
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

void cearch_printf_type(Cearch_Data_Type *type) {
    if (type == NULL) printf("(untyped)");

    switch (type->kind) {
        case CDTK_NIL:
            printf("nil");
            break;
        case CDTK_INT:
            printf("int");
            break;
        case CDTK_FLOAT:
            printf("float");
            break;
        case CDTK_STR:
            printf("str");
            break;
        case CDTK_BOOL:
            printf("bool");
            break;
        case CDTK_ARRAY:
            printf("array<");
            cearch_printf_type(type->inner);
            printf(">");
            break;
        default: assert(0 && "cearch_printf_type: missing handler for Cearch_Data_Type"); break;
    }

    if (type->nullable) printf("?");
}

enum tk {
    tk_int_type   = CDTK_INT,
    tk_float_type = CDTK_FLOAT,
    tk_str_type   = CDTK_STR,
    tk_bool_type  = CDTK_BOOL,
    tk_array_type = CDTK_ARRAY,
    tk_lt_type,
    tk_gt_type,
    tk_qmark_type,
};

Cearch_Data_Type *cearch_parse_data_type(const char *function_name, const char *type_string) {
    int length = strlen(type_string);

    static const int m = 16;                // deepest array level allowed
    static const int a = 5;                 // len('array')
    static const int t = 5;                 // len('float') : longest type name size
    static const int X = a * m + t + m * 2; // m * 2 = ('<' * m) + ('>' * m)
    static const int max_amount_of_tokens = m * 2 + m + 1; // ('<' * m) + ('>' * m) + ('array' * m) + 'primitive';

    if (length > X) {
        display_error_and_exit(
            type_string,
            function_name,
            "type definition is too long",
            "max type string length is %d",
            0,
            0,
            X
        );
    }

    int tokens[max_amount_of_tokens];
    int tokens_length = 0;

    int bot = 0, cursor = 0;

    while (cursor < length) {
        if (tokens_length >= max_amount_of_tokens) {
            display_error_and_exit(
                type_string,
                function_name,
                "maximum number of tokens reached",
                "the maximum number of tokens is %d",
                0,
                0,
                max_amount_of_tokens
            );
        }

        bot = cursor;

        char c = type_string[cursor];

        if (c == '>') {
            tokens[tokens_length++] = tk_gt_type;
        } else if (c == '<') {
            tokens[tokens_length++] = tk_lt_type;
        } else if (c == '?') {
            tokens[tokens_length++] = tk_qmark_type;
        } else if (is_type_name(c)) {
            while (cursor < length && is_type_name(type_string[cursor])) cursor++;

            int size = cursor - bot - 1;

            if (strncmp("array", type_string + bot, size) == 0) {
                tokens[tokens_length++] = tk_array_type;
            } else if (strncmp("int", type_string + bot, size) == 0) {
                tokens[tokens_length++] = tk_int_type;
            } else if (strncmp("str", type_string + bot, size) == 0) {
                tokens[tokens_length++] = tk_str_type;
            } else if (strncmp("bool", type_string + bot, size) == 0) {
                tokens[tokens_length++] = tk_bool_type;
            } else if (strncmp("float", type_string + bot, size) == 0) {
                tokens[tokens_length++] = tk_float_type;
            } else {
                display_error_and_exit(
                    type_string,
                    function_name,
                    "invalid data type",
                    "expected 'int', 'str', 'bool', 'float' or 'array' but got '%.*s'",
                    bot,
                    cursor,
                    cursor - bot,
                    type_string + bot
                );
            }

            continue;
        } else {
            display_error_and_exit(
                type_string,
                function_name,
                "invalid type string",
                "unexpected '%c'",
                cursor,
                cursor,
                c
            );
        }

        cursor++;
    }

    if (tokens_length == 0) {
        display_error_and_exit(
            type_string,
            function_name,
            "invalid type string",
            "you cannot have an empty type string",
            -1,
            -1
        );
    }

    return NULL;
}
