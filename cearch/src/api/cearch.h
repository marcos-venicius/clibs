#ifndef _CEARCH_API_H_
#define _CEARCH_API_H_

typedef struct Cearch Cearch;

Cearch *cearch_init(void);
/**
 * How to describe variable types?
 *
 * That's all the primitive types:
 *  int
 *  float
 *  str
 *  bool
 * If your variable is just a primitive type, just specify it.
 *  cearch_define_variable(&cearch, "status", "int");
 * You can also have arrays.
 * To specify arrays you can use the following syntax:
 *  array<primitive>
 * Examples:
 *  cearch_define_variable(&cearch, "roles", "array<str>");
 *  cearch_define_variable(&cearch, "embed", "array<array<float>>");
 * If your primitive is nullable, you have the '?' notation.
 * Examples:
 *  cearch_define_variable(&cearch, "user_agent", "str?");
 * !!!! You cannot have nullable arrays or nullables inside arrays !!!!
 * That syntax is invalid: "array<int>?", and that one too ""
 */
void cearch_define_variable(Cearch *cearch, const char *variable_name, const char *variable_type);
void cearch_debug_variables(Cearch *cearch);

#endif // _CEARCH_API_H_
