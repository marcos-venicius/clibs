#include "./types.h"

#include <stdio.h>
#include <assert.h>

bool cearch_compare_types(Cearch_Data_Type *left, Cearch_Data_Type *right) {
    if (left == NULL && right == NULL) return true;
    if (left == NULL || right == NULL) return false;
    if (left->kind != right->kind) return false;

    if (left->kind == CDTK_ARRAY) {
        assert(left->inner != NULL && "left->etype should not be null when dtype is CDTK_ARRAY");
        assert(right->inner != NULL && "right->etype should not be null when dtype is CDTK_ARRAY");

        return cearch_compare_types(left->inner, right->inner);
    }

    return false;
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
}
