#ifndef _CEARCH_TYPES_H_
#define _CEARCH_TYPES_H_

#include <stdbool.h>

typedef enum {
    CDTK_NIL,
    CDTK_INT,
    CDTK_FLOAT,
    CDTK_STR,
    CDTK_BOOL,
    CDTK_ARRAY,
} Cearch_Data_Type_Kind;

typedef struct Cearch_Data_Type Cearch_Data_Type;

struct Cearch_Data_Type {
    Cearch_Data_Type_Kind kind;

    // only used if kind is CDTK_ARRAY
    Cearch_Data_Type *inner;
};

bool cearch_compare_types(Cearch_Data_Type *left, Cearch_Data_Type *right);
void cearch_printf_type(Cearch_Data_Type *type);

#endif // _CEARCH_TYPES_H_
