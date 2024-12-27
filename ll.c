#include "assertf.h"
#include "ll.h"
#include <stdlib.h>
#include <string.h>

// callbacks

bool int_compare(void *a, void *b) {
    return *(int*)a == *(int*)b;
}

bool string_compare(void *a, void *b) {
    char *as = (char*)a;
    char *bs = (char*)b;

    return strncmp(as, bs, strlen(as)) == 0;
}

LL *ll_new(FreeCallback free_callback, CompareCallback compare_callback) {
    LL *ll = calloc(1, sizeof(LL));

    ll->compare_callback = compare_callback;
    ll->free_callback = free_callback;
    ll->version = 1;

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
    ll->version++;
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

    for (size_t i = 0; i < index; ++i) {
        slow = fast;
        fast = fast->next;
    }

    // middle
    if (slow != NULL && fast != NULL) {
        slow->next = fast->next;
    }

    // if it's the first item, update the head
    if (index == 0) {
        ll->head = ll->head->next;
    }

    // if it's the last item update the tail. It also works when the last is also the first one
    // meaning a list with only 1 item. the slow will be null then we say that tail will be null.
    if (index + 1 == ll->count) {
        ll->tail = slow;
    }

    if (ll->free_callback != NULL) {
        ll->free_callback(fast->data);
    } else {
        free(fast->data);
    }

    free(fast);

    ll->count--;
    ll->version++;
}

void ll_remove_by_value(LL *ll, void *data) {
    assertf(ll != NULL, "ll is NULL");
    assertf(ll->compare_callback != NULL, "you should have a compare callback when removing by value");
    assertf(ll->count > 0, "there is not value inside the list");

    LLNode *slow = NULL;
    LLNode *fast = ll->head;

    bool found = false;
    size_t index = 0;

    while (fast != NULL) {
        if (ll->compare_callback(fast->data, data)) {
            found = true;
            break;
        }

        slow = fast;
        fast = fast->next;

        ++index;
    }

    if (!found) return;

    // middle
    if (slow != NULL && fast != NULL) {
        slow->next = fast->next;
    }

    // if it's the first item, update the head
    if (index == 0) {
        ll->head = ll->head->next;
    }

    // if it's the last item update the tail. It also works when the last is also the first one
    // meaning a list with only 1 item. the slow will be null then we say that tail will be null.
    if (index + 1 == ll->count) {
        ll->tail = slow;
    }

    if (ll->free_callback != NULL) {
        ll->free_callback(fast->data);
    } else {
        free(fast->data);
    }

    free(fast);

    ll->count--;
    ll->version++;
}

void ll_remove_by_value_i(LL *ll, int i) {
    ll_remove_by_value(ll, &i);
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
        .index = 0,
        .version = ll->version
    };
}

bool ll_iter_has(LLIter *iter) {
    assertf(iter != NULL, "iter cannot be null");
    assertf(iter->version == iter->ll->version, "Are you forgoting to call ll_iter_flush after some updates on ll?");

    return iter->index < iter->ll->count;
}

LLIterItem ll_iter_consume(LLIter *iter) {
    assertf(iter != NULL, "iter cannot be null");
    assertf(iter->current != NULL, "you reached the end of this iteration");
    assertf(iter->version == iter->ll->version, "Are you forgoting to call ll_iter_flush after some updates on ll?");

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
    iter->version = iter->ll->version;
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
