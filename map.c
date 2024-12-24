#include "map.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

size_t hash_key(char *key) {
    unsigned long hash = 5381;

    int i = 0;

    while (key[i] != '\0') {
        hash = ((hash << 5) + hash) + (int)(key[i] - '0');
        i++;
    }

    return hash % MAP_BUCKET_SIZE;
}

void realloc_node_data(MapNode *node, size_t data_size) {
    if (data_size == 0 || node->data == NULL) return;

    if (node->data != NULL) {
        node->data = realloc(node->data, data_size);

        if (node->data == NULL) {
            fprintf(stderr, "Error: Failed to reallocate memory\n");
            exit(EXIT_FAILURE);
        }
    } else {
        node->data = malloc(data_size);

        if (node->data == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
    }
}

MapNode *new_node(Map* map, char *key, void *data, size_t data_size) {
    MapNode *node = calloc(1, sizeof(MapNode));

    node->key = malloc(strlen(key) + 1);
    strcpy(node->key, key);

    node->next = NULL;

    if (data != NULL) {
        node->data = malloc(data_size);

        memcpy(node->data, data, data_size);
    }

    map->length++;

    return node;
}

Map *map_new() {
    return calloc(1, sizeof(Map));
}

void map_set(Map *map, char *key, void *data, size_t data_size) {
    size_t index = hash_key(key);

    MapNode *current = map->nodes[index];

    if (current == NULL) {
        map->nodes[index] = new_node(map, key, data, data_size);

        return;
    }

    while (current != NULL) {
        if (strncmp(current->key, key, strlen(key)) == 0) {
            realloc_node_data(current, data_size);
            memcpy(current->data, data, data_size);

            return;
        }

        if (current->next == NULL) break;

        current = current->next;
    }

    current->next = new_node(map, key, data, data_size);
}

void map_set_i(Map *map, char *key, int i) {
    map_set(map, key, &i, sizeof(int));
}

void map_set_s(Map *map, char *key, char *s) {
    map_set(map, key, s, strlen(s) + 1);
}

void *map_get(Map *map, char *key) {
    size_t index = hash_key(key);

    MapNode *current = map->nodes[index];

    while (current != NULL && (strncmp(current->key, key, strlen(key)))) {
        current = current->next;
    }

    return current != NULL ? current->data : NULL;
}

void map_free(Map *map) {
    for (size_t i = 0; i < MAP_BUCKET_SIZE; i++) {
        MapNode *current = map->nodes[i];

        if (current != NULL) {
            while (current != NULL) {
                MapNode *next = current->next;

                free(current->key);
                free(current->data);
                free(current);

                current = next;
            }
        }
    }

    free(map);
}
