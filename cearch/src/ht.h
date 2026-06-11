#ifndef _CEARCH_HT_H_
#define _CEARCH_HT_H_

#define _HT_SIZE 1019

typedef struct Ht_Node Ht_Node;

struct Ht_Node {
    struct {
        char *key;
        int length;
    } name;
    void* data;
    Ht_Node* next;
};

typedef struct {
    int length;
    int data_size;
    Ht_Node* nodes[_HT_SIZE];
} Ht;

typedef struct {
    Ht*        ht;  // hash table
    Ht_Node* it;  // current node on iteration
    int        idx; // current nodes array position on iteration
} Ht_Iterator;

Ht*     ht_init(int data_size);
void    ht_add(Ht* ht, const char* key, void* data);
void*   ht_find(Ht* ht, const char* key);
void    ht_free(Ht* ht);

Ht_Iterator ht_iterator(Ht* ht);
Ht_Node* ht_iterator_next(Ht_Iterator* it);

#endif // _CEARCH_HT_H_
