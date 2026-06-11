#ifndef _CEARCH_TYPECHECKER_H_
#define _CEARCH_TYPECHECKER_H_

#include "./parser.h"
#include "./types.h"
#include "./ht.h"

void cearch_data_type_infer(Cearch_Ast_Node *ast, Ht *variables_types);

#endif // _CEARCH_TYPECHECKER_H_
