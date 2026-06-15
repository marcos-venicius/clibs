#include <assert.h>

#include "./token.h"

const char *cearch_token_kind_enum_name(cearch_token_kind_enum_t kind) {
    switch (kind) {
        case CTK_NIL: return "nil";
        case CTK_BOOL: return "bool";
        case CTK_STR: return "str";
        case CTK_INT: return "int";
        case CTK_FLOAT: return "float";

        case CTK_LSQUARE: return "[";
        case CTK_RSQUARE: return "]";
        case CTK_LPAREN: return "(";
        case CTK_RPAREN: return ")";
        case CTK_COMMA: return ",";
        case CTK_DOT: return ".";
        case CTK_LT: return "<";
        case CTK_GT: return ">";
        case CTK_LTE: return "<=";
        case CTK_GTE: return ">=";
        case CTK_EQ: return "=";
        case CTK_NEQ: return "!=";
        case CTK_NOT: return "!";

        case CTK_OR: return "or";
        case CTK_AND: return "and";

        case CTK_SYM: return "sym";

        case CTK_EOF: return "eof";

        default: assert(0 && "cearch_token_kind_enum_name: missing cearch_token_kind_enum_t");
    }
}
