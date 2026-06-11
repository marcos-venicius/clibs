#include "./ht.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

static int hash_key(const char *key) {
    const unsigned char *str = (const unsigned char*)key;

    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) hash = ((hash << 5) + hash) + c;

    return hash % _HT_SIZE;
}

static Ht_Node* alloc_node(Ht* ht, const char *key, int key_length, void *data) {
    Ht_Node* node = malloc(sizeof(Ht_Node));

    // I'm using strdup (which includes the null-byte) because
    // we may want provide a loop for the client with all keys and values.
    // I don't want to mess up and let a segfault happen just because
    // the called is not concerned about this.
    node->name.key = strdup(key);
    node->name.length = key_length;
    node->next = NULL;
    node->data = malloc(ht->data_size);

    memcpy(node->data, data, ht->data_size);

    return node;
}

static inline void free_node(Ht_Node* node) {
    free(node->data);
    free(node->name.key);
    free(node);
}

Ht* ht_init(int data_size) {
    Ht* ht = calloc(1, sizeof(Ht));

    assert(data_size > 0 && "'data_size' should be greater than zero");

    ht->data_size = data_size;

    return ht;
}

void ht_add(Ht *ht, const char *key, void *data) {
    // TODO: we're not rehashing the keys,
    //       we may do it in the future, but for the current
    //       purpose of this DS we don't need it.
    int index = hash_key(key);
    int key_length = strlen(key);

    if (ht->nodes[index] == NULL) {
        ht->nodes[index] = alloc_node(ht, key, key_length, data);
    } else {
        Ht_Node* slow = NULL;
        Ht_Node* fast = ht->nodes[index];

        while (fast != NULL) {
            // exact same key
            if (fast->name.length == key_length && (memcmp(fast->name.key, key, key_length) == 0)) {
                // substituted node with exact same key
                memcpy(fast->data, data, ht->data_size);
                return;
            }

            slow = fast;
            fast = fast->next;
        }

        // added new node at the end
        slow->next = alloc_node(ht, key, key_length, data);
    }
}

void* ht_find(Ht* ht, const char* key) {
    int index = hash_key(key);
    int key_length = strlen(key);

    Ht_Node* curr = ht->nodes[index];

    while (curr != NULL) {
        if (curr->name.length == key_length && (memcmp(curr->name.key, key, key_length) == 0))
            return curr->data;

        curr = curr->next;
    }

    return NULL;
}

void ht_free(Ht* ht) {
    // TODO: use arena?
    for (int i = 0; i < _HT_SIZE; i++) {
        Ht_Node* head = ht->nodes[i];

        while (head != NULL) {
            Ht_Node* next = head->next;

            free_node(head);

            head = next;
        }
    }

    free(ht);
}


// ITERATOR STUFF

Ht_Iterator ht_iterator(Ht* ht) {
    return (Ht_Iterator){
        .ht = ht,
        .it = NULL,
        .idx = 0
    };
}

Ht_Node* ht_iterator_next(Ht_Iterator* it) {
    if (it == NULL || (it->idx >= _HT_SIZE && it->it == NULL)) return NULL;

    if (it->it != NULL) {
        if (it->it->next != NULL)
            return it->it = it->it->next;
        else
            it->idx++;
    }

    while (it->idx < _HT_SIZE) {
        if (it->ht->nodes[it->idx] != NULL) {
            it->it = it->ht->nodes[it->idx];

            return it->it;
        }

        it->idx++;
    }

    it->it = NULL;

    return NULL;
}
