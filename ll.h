#ifndef CL_LL_H_
#define CL_LL_H_
#include <stddef.h>
#include <stdbool.h>

typedef bool (*CompareCallback)(void *a, void *b);
typedef void (*FreeCallback)(void *);

typedef struct LLNode LLNode;

struct LLNode {
    void *data;
    LLNode *next;
};

typedef struct {
    LLNode *head;
    LLNode *tail;

    size_t count;

    // callbacks
    CompareCallback compare_callback;
    FreeCallback free_callback;
} LL;

typedef struct {
  LL *ll;
  LLNode *current;
  size_t index;
} LLIter;

typedef struct {
  void *data;
  size_t index;
} LLIterItem;

// default free callback is "free"
LL *ll_new(FreeCallback free_callback, CompareCallback compare_callback);

bool int_compare(void *a, void *b);
bool string_compare(void *a, void *b);

// If the data size is 0, instead of allocating memory to the data
// we just pointer to it
void ll_add(LL *ll, void *data, size_t data_size);
void ll_add_i(LL *ll, int i);
void ll_add_s(LL *ll, char *s);

void ll_remove_by_value(LL *ll, void *data);
void ll_remove_by_value_i(LL *ll, int i);
void ll_remove_by_index(LL *ll, size_t index); // TODO: allow -1, -2, ... as index

void *ll_find_by_value(LL *ll, void *data);
void *ll_find_by_index(LL *ll, size_t index); // TODO: allow -1, -2, ... as index

LLIter ll_iter(LL *ll);
bool ll_iter_has(LLIter *iter);
LLIterItem ll_iter_consume(LLIter *iter);
void ll_iter_flush(LLIter *iter);

void ll_free(LL *ll);

#endif // CL_LL_H_

