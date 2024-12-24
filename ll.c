#include "assertf.h"
#include "ll.h"
#include <stdlib.h>
#include <string.h>

LL *ll_new(FreeCallback free_callback, CompareCallback compare_callback) {
    LL *ll = calloc(1, sizeof(LL));

    ll->compare_callback = compare_callback;
    ll->free_callback = free_callback;

    return ll;
}

void ll_add(LL *ll, void *data, size_t data_size) {
    assertf(ll != NULL, "ll is NULL");

    LLNode *node = malloc(sizeof(LLNode));

    node->next = NULL;
    node->data = NULL;

    if (data != NULL) {
        if (data_size == 0) {
            node->data = data;
        } else {
            node->data = malloc(data_size);

            memcpy(node->data, data, data_size);
        }
    }

    if (ll->head == NULL) {
        ll->head = node;
        ll->tail = node;
    } else {
        ll->tail->next = node;
        ll->tail = ll->tail->next;
    }

    ll->count++;
}

void ll_add_i(LL *ll, int i) {
    ll_add(ll, &i, sizeof(int));
}

void ll_add_s(LL *ll, char *s) {
    ll_add(ll, s, strlen(s) + 1);
}

void ll_remove_by_index(LL *ll, size_t index) {
    assertf(ll != NULL, "ll is NULL");
    assertf(index < ll->count, "index %ld out of range. current list size is %ld", index, ll->count);

    LLNode *slow = NULL;
    LLNode *fast = ll->head;

    size_t i = 0;

    // TODO: what if it's the last node?
    // TODO: what if fast is null?
    while (i++ < index) {
        slow = fast;
        fast = fast->next;
    }

    slow->next = fast->next;

    if (ll->free_callback != NULL) {
        ll->free_callback(fast->data);
    } else {
        free(fast->data);
    }

    free(fast);

    ll->count--;
}

void ll_remove_by_value(LL *ll, void *data) {
    assertf(ll != NULL, "ll is NULL");
    assertf(ll->compare_callback != NULL, "you should have a compare callback when removing by value");

    LLNode *slow = NULL;
    LLNode *fast = ll->head;

    // TODO: what if it's the last node?
    while (fast != NULL) {
        if (ll->compare_callback(fast->data, data)) {
            if (slow == NULL) {
                LLNode* next = ll->head->next;

                if (ll->free_callback != NULL) {
                    ll->free_callback(ll->head->data);
                } else {
                    free(ll->head->data);
                }

                free(ll->head);

                ll->head = next;

                if (ll->head->next == NULL) {
                    ll->tail = ll->head;
                }
            } else {
                slow->next = fast->next;

                if (ll->free_callback != NULL) {
                    ll->free_callback(fast->data);
                } else {
                    free(fast->data);
                }

                free(fast);

                if (slow->next == NULL) {
                    ll->tail = slow;
                }
            }

            ll->count--;
            return;
        }

        slow = fast;
        fast = fast->next;
    }
}

void *ll_find_by_index(LL *ll, size_t index) {
    assertf(ll != NULL, "ll is NULL");
    assertf(index < ll->count, "index %ld out of range. current list size is %ld", index, ll->count);

    LLNode *current = ll->head;

    for (size_t i = 0; i < index; i++) {
        current = current->next;
    }

    return current->data;
}

void *ll_find_by_value(LL *ll, void *data) {
    assertf(ll != NULL, "ll is NULL");
    assertf(ll->compare_callback != NULL, "you need to have a compare_callback to find by value");

    if (ll->count == 0) return NULL;

    LLNode *current = ll->head;

    while (current != NULL) {
        if (ll->compare_callback(current->data, data)) {
            return current->data;
        }

        current = current->next;
    }

    return NULL;
}

LLIter ll_iter(LL *ll) {
    assertf(ll != NULL, "ll cannot be null");

    return (LLIter){
        .ll = ll,
        .current = ll->head,
        .index = 0
    };
}

bool ll_iter_has(LLIter *iter) {
    assertf(iter != NULL, "iter cannot be null");

    return iter->index < iter->ll->count;
}

LLIterItem ll_iter_consume(LLIter *iter) {
    assertf(iter != NULL, "iter cannot be null");
    assertf(iter->current != NULL, "you reached the end of this iteration");

    LLNode *node = iter->current;

    LLIterItem item = {
        .data = node->data,
        .index = iter->index
    };

    iter->current = iter->current->next;
    iter->index++;

    return item;
}

void ll_iter_flush(LLIter *iter) {
    assertf(iter != NULL, "iter cannot be null");

    iter->current = iter->ll->head;
    iter->index = 0;
}

void ll_free(LL *ll) {
    if (ll == NULL) return;

    if (ll->head == NULL) {
        free(ll);

        return;
    }

    LLNode *current = ll->head;

    while (current != NULL) {
        LLNode *next = current->next;

        if (current->data != NULL) {
            if (ll->free_callback == NULL) {
                free(current->data);
            } else {
                ll->free_callback(current->data);
            }
        }

        free(current);

        current = next;
    }

    free(ll);
}
