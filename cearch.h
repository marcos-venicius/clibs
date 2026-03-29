#ifndef CL_CEARCH_H_
#define CL_CEARCH_H_
#include <stdbool.h>
#include <stddef.h>

/**

Missing types:
    - Integers
    - Floats

Missing Implementations:
    - String escaping
    - UTF-8 Strings

*/

#define CL_CEARCH_IMPLEMENTATION

// arbitrary numbers for now
#define CL_CEARCH_MAX_BOOLEAN_VARIABLES 15
#define CL_CEARCH_MAX_SINGLE_VALUE_ATOM_VARIABLES 15
#define CL_CEARCH_MAX_MULTIPLE_VALUES_ATOM_VARIABLES 15
#define CL_CEARCH_MAX_MULTIPLE_VALUES_ATOM_VARIABLE_VALUES 15

typedef enum {
    __cearch_token_kind_left_paren = '(',
    __cearch_token_kind_right_paren = ')',
    __cearch_token_kind_atom = ':',
    __cearch_token_kind_var = 'v',
    __cearch_token_kind_comma = ',',
    __cearch_token_kind_or = '|',
    __cearch_token_kind_and = '&',
    __cearch_token_kind_not = '!',
    __cearch_token_kind_in = 'i',
    __cearch_token_kind_contains = 'c',
} __cearch_token_kind_t;

typedef struct __cearch_token_t __cearch_token_t;

struct __cearch_token_t {
    __cearch_token_kind_t kind;

    char *content;
    size_t size;
    size_t col;

    __cearch_token_t *next;
};

typedef struct {
    char    *content;
    size_t  content_length;
    size_t  cursor;
    size_t  bot;
    bool    has_error;
    int     stage;

    __cearch_token_t *tokens_head;
    __cearch_token_t *tokens_tail;
} __state_t;

typedef struct {
    const char *name;
    bool value;
} CL_Cearch_Boolean_Variable;

typedef struct {
    const char *name;
    const char *value;
} CL_Cearch_Single_Value_Atom_Variable;

typedef struct {
    const char *name;
    const char *value[CL_CEARCH_MAX_MULTIPLE_VALUES_ATOM_VARIABLE_VALUES];
    int        values_count;
} CL_Cearch_Multiple_Values_Atom_Variable;

typedef struct {
    CL_Cearch_Boolean_Variable boolean_variables[CL_CEARCH_MAX_BOOLEAN_VARIABLES];
    int                        boolean_variables_count;

    CL_Cearch_Single_Value_Atom_Variable single_value_atom_variables[CL_CEARCH_MAX_SINGLE_VALUE_ATOM_VARIABLES];
    int                                  single_value_atom_variables_count;

    CL_Cearch_Multiple_Values_Atom_Variable multiple_values_atom_variables[CL_CEARCH_MAX_MULTIPLE_VALUES_ATOM_VARIABLES];
    int                                     multiple_values_atom_variables_count;


    __state_t __state;
} CL_Cearch;

/**
 * PUBLIC API
 */

/**
 * If you call this function multiple times, it will always overwrite the preivous value
 */
void cl_cearch_set_boolean_variable(CL_Cearch *cearch, const char *name, bool value);
/**
 * If you call this function multiple times, it will always overwrite the preivous value
 */
void cl_cearch_set_single_value_atom_variable(CL_Cearch *cearch, const char *name, const char *value);
/**
 * Just call this function multiple times and it'll keep adding the values
 */
void cl_cearch_set_multiple_values_atom_variable(CL_Cearch *cearch, const char *name, const char *value);

bool cl_cearch_compile(CL_Cearch *cearch, char *search);
bool cl_cearch_match(const CL_Cearch *cearch);
void cl_cearch_free(CL_Cearch *cearch);

void cl_cearch_dump_tokens(CL_Cearch *cearch);

/*
    // C Code

    CL_Cearch cearch = {0};

    cl_cearch_set_boolean_variable(&cearch, "is_reminder", true);
    cl_cearch_set_single_value_atom_variable(&cearch, "state", "todo");
    cl_cearch_set_multiple_values_atom_variable(&cearch, "tags", "high_priority");
    cl_cearch_set_multiple_values_atom_variable(&cearch, "tags", "wodo_project");

    if (!cl_cearch_compile(&cearch,  "(state not in (:blocked, :done) and is_reminder) or tags contains :high_priority")) {
        cl_cearch_free(&cearch);

        return 1;
    }

    if (cl_cearch_match(&cearch) {
        printf("matched\n");
    } else {
        printf("did not match\n");
    }

    // should print "matched"
*/

#ifdef CL_CEARCH_IMPLEMENTATION
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// 'compiling' stages
#define __cl_cearch_waiting_tokenization_stage 0
#define __cl_cearch_tokenizing_stage           1
#define __cl_cearch_waiting_parsing_stage      2
#define __cl_cearch_parsing_stage              3
#define __cl_cearch_parsed_stage               4

#define __new_token(n) __cearch_token_t *n = __alloc_token()

static bool __cmp_sized_with_cstring(const char *sized, size_t sized_s, const char *cstr) {
    size_t cstr_s = strlen(cstr);

    if (cstr_s != sized_s) return false;

    return strncmp(sized, cstr, sized_s) == 0;
}

static const char *__token_kind_name(__cearch_token_kind_t kind) {
    switch (kind) {
        case __cearch_token_kind_left_paren: return "left_paren";
        case __cearch_token_kind_right_paren: return "right_paren";
        case __cearch_token_kind_atom: return "atom";
        case __cearch_token_kind_var: return "var";
        case __cearch_token_kind_comma: return "comma";
        case __cearch_token_kind_or: return "or";
        case __cearch_token_kind_and: return "and";
        case __cearch_token_kind_not: return "not";
        case __cearch_token_kind_in: return "in";
        case __cearch_token_kind_contains: return "contains";
        default: assert(0 && "unhandled __cearch_token_kind_t at __token_kind_name");
    }
}

static inline void __add_token(CL_Cearch *cearch, __cearch_token_t *token) {
    if (cearch->__state.tokens_head == NULL) {
        cearch->__state.tokens_tail = cearch->__state.tokens_head = token;
    } else {
        cearch->__state.tokens_tail = cearch->__state.tokens_tail->next = token;
    }
}

static inline void __advance_lexer(CL_Cearch *cearch) {
    if (cearch->__state.cursor < cearch->__state.content_length) cearch->__state.cursor++;
}

static inline char __chr(const CL_Cearch *cearch) {
    return cearch->__state.cursor < cearch->__state.content_length ? cearch->__state.content[cearch->__state.cursor] : '\0';
}

static inline bool __is_var(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static inline bool __is_whitespace(char c) {
    return c == ' ' || c == '\t';
}

static inline __cearch_token_t *__alloc_token() {
    return (__cearch_token_t *)malloc(sizeof(__cearch_token_t));
}

static void __error(CL_Cearch *cearch, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    static int pad_left = 7;

    int content_length = cearch->__state.content_length;
    int start = cearch->__state.bot;
    int end = cearch->__state.cursor;
    int middle = end - start;

    if (start != end)
        fprintf(stderr, "error (%d-%d): ", start, end);
    else
        fprintf(stderr, "error (%d): ", start);

    int si = start - 5 > 0 ? 5 : 0;
    int ei = end + 5 >= content_length ? content_length : 5;

    start = start - si;
    end = end + ei;
    int size = end - start;

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n\n");
    fprintf(stderr, "%*.s\033[1;31m%.*s\033[0m\n", pad_left, "", size, cearch->__state.content + start);
    fprintf(stderr, "%*.s^\n", pad_left + middle + si, "");

    va_end(args);

    cearch->__state.has_error = true;
}

static inline void __consume_single_token(CL_Cearch *cearch, __cearch_token_kind_t kind) {
    __new_token(token);

    token->kind = kind;
    token->content = cearch->__state.content + cearch->__state.bot;
    token->size = 1;
    token->next = NULL;
    token->col = cearch->__state.bot;

    __add_token(cearch, token);
    __advance_lexer(cearch);
}

static void __consume_variable(CL_Cearch *cearch) {
    while (__is_var(__chr(cearch))) __advance_lexer(cearch);

    __new_token(token);

    token->kind = __cearch_token_kind_var;
    token->content = cearch->__state.content + cearch->__state.bot;
    token->size = cearch->__state.cursor - cearch->__state.bot;
    token->next = NULL;
    token->col = cearch->__state.bot;

    if (__cmp_sized_with_cstring(token->content, token->size, "or"))
        token->kind = __cearch_token_kind_or;
    else if (__cmp_sized_with_cstring(token->content, token->size, "and"))
        token->kind = __cearch_token_kind_and;
    else if (__cmp_sized_with_cstring(token->content, token->size, "contains"))
        token->kind = __cearch_token_kind_contains;
    else if (__cmp_sized_with_cstring(token->content, token->size, "in"))
        token->kind = __cearch_token_kind_in;
    else if (__cmp_sized_with_cstring(token->content, token->size, "not"))
        token->kind = __cearch_token_kind_not;

    __add_token(cearch, token);
}

static void __consume_whitespaces(CL_Cearch *cearch) {
    while (__is_whitespace(__chr(cearch))) __advance_lexer(cearch);
}

static void __consume_atom(CL_Cearch *cearch) {
    __advance_lexer(cearch);

    int size = 0;

    while (__is_var(__chr(cearch))) (size++, __advance_lexer(cearch));

    if (size == 0) {
        __error(cearch, "invalid atom");
    }

    __new_token(token);

    token->kind = __cearch_token_kind_atom;
    token->content = cearch->__state.content + cearch->__state.bot + 1;
    token->size = cearch->__state.cursor - cearch->__state.bot - 1;
    token->next = NULL;
    token->col = cearch->__state.bot;

    __add_token(cearch, token);
}

static bool __tokenize(CL_Cearch *cearch) {
    assert(cearch->__state.stage == __cl_cearch_waiting_tokenization_stage && "you cannot run tokenizer at this stage");

    cearch->__state.stage = __cl_cearch_tokenizing_stage;

    while (cearch->__state.cursor < cearch->__state.content_length) {
        __consume_whitespaces(cearch);

        cearch->__state.bot = cearch->__state.cursor;

        const char c = __chr(cearch);

        switch (c) {
            case __cearch_token_kind_left_paren: __consume_single_token(cearch, __cearch_token_kind_left_paren); break;
            case __cearch_token_kind_right_paren: __consume_single_token(cearch, __cearch_token_kind_right_paren); break;
            case __cearch_token_kind_comma: __consume_single_token(cearch, __cearch_token_kind_comma); break;
            case __cearch_token_kind_atom: __consume_atom(cearch); break;
            default: {
                if (__is_var(c)) {
                    __consume_variable(cearch);
                } else {
                    __error(cearch, "unrecognized character '%c'", c);
                    return false;
                }
            } break;
        }
    }

    cearch->__state.stage = __cl_cearch_waiting_parsing_stage;

    return true;
}

static bool __parse(CL_Cearch *cearch) {
    assert(cearch->__state.stage == __cl_cearch_waiting_parsing_stage && "you cannot run parser at this stage");

    cearch->__state.stage = __cl_cearch_parsing_stage;

    // TODO: start parsing expression

    __error(cearch, "here parser was not initialized yet");

    cearch->__state.stage = __cl_cearch_parsed_stage;

    return false;
}

void cl_cearch_set_boolean_variable(CL_Cearch *cearch, const char *name, bool value) {
    if (!cearch || !name) return;

    for (int i = 0; i < cearch->boolean_variables_count; ++i) {
        if (strcmp(name, cearch->boolean_variables[i].name) == 0) {
            cearch->boolean_variables[i].value = value;

            return;
        }
    }

    assert(cearch->boolean_variables_count < CL_CEARCH_MAX_BOOLEAN_VARIABLES && "you reached max boolean variables");

    cearch->boolean_variables[cearch->boolean_variables_count++] = (CL_Cearch_Boolean_Variable){
        .name = name,
        .value = value
    };
}

void cl_cearch_set_single_value_atom_variable(CL_Cearch *cearch, const char *name, const char *value) {
    for (int i = 0; i < cearch->single_value_atom_variables_count; ++i) {
        if (strcmp(name, cearch->single_value_atom_variables[i].name) == 0) {
            cearch->single_value_atom_variables[i].value = value;

            return;
        }
    }

    assert(cearch->single_value_atom_variables_count < CL_CEARCH_MAX_SINGLE_VALUE_ATOM_VARIABLES && "you reached max single-atom variables");

    cearch->single_value_atom_variables[cearch->single_value_atom_variables_count++] = (CL_Cearch_Single_Value_Atom_Variable){
        .name = name,
        .value = value
    };
}

void cl_cearch_set_multiple_values_atom_variable(CL_Cearch *cearch, const char *name, const char *value) {
    for (int i = 0; i < cearch->multiple_values_atom_variables_count; ++i) {
        if (strcmp(name, cearch->multiple_values_atom_variables[i].name) == 0) {
            for (int j = 0; j < cearch->multiple_values_atom_variables[i].values_count; ++j)
                if (strcmp(cearch->multiple_values_atom_variables[i].value[j], value) == 0) return;

            assert(cearch->multiple_values_atom_variables->values_count < CL_CEARCH_MAX_MULTIPLE_VALUES_ATOM_VARIABLE_VALUES && "you reached max multiple-atom variable values");

            cearch->multiple_values_atom_variables->value[cearch->multiple_values_atom_variables->values_count++] = value;

            return;
        }
    }

    assert(cearch->multiple_values_atom_variables_count < CL_CEARCH_MAX_MULTIPLE_VALUES_ATOM_VARIABLES && "you reached max multiple-atom variables");

    CL_Cearch_Multiple_Values_Atom_Variable variable = {0};

    variable.name = name;
    variable.values_count = 0;
    variable.value[variable.values_count++] = value;

    cearch->multiple_values_atom_variables[cearch->multiple_values_atom_variables_count++] = variable;
}

bool cl_cearch_compile(CL_Cearch *cearch, char *search) {
    cearch->__state.content = search;
    cearch->__state.content_length = strlen(search);
    cearch->__state.cursor = 0;
    cearch->__state.bot = 0;

    if (!__tokenize(cearch)) return false;
    if (!__parse(cearch)) return false;

    return cearch->__state.has_error == false;
}

bool cl_cearch_match(const CL_Cearch *cearch) {
    (void)cearch;
    // TODO: evaluate the AST to a boolean

    return false;
}

void cl_cearch_free(CL_Cearch *cearch) {
    __cearch_token_t *curr = cearch->__state.tokens_head;

    while (curr != NULL) {
        __cearch_token_t *next = curr->next;

        free(curr);

        curr = next;
    }
}

void cl_cearch_dump_tokens(CL_Cearch *cearch) {
    __cearch_token_t *curr = cearch->__state.tokens_head;

    while (curr != NULL) {
        printf("<%s value='%.*s' />\n", __token_kind_name(curr->kind), (int)curr->size, curr->content);

        curr = curr->next;
    }
}

#endif // !CL_CEARCH_IMPLEMENTATION
#endif // !CL_CEARCH_H_
