#include <stdio.h>
#include <stdlib.h>
#include "ll.h"

typedef struct {
    int x, y;
} Vec2;

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Missing width and height\n");
        fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
        fprintf(stderr, "Example: %s 5 5\n", argv[0]);
        exit(1);
    }

    int w = atoi(argv[1]);
    int h = atoi(argv[2]);

    LL *positions = ll_new((FreeCallback)ll_free, NULL);

    for (int y = 0; y < h; ++y) {
        LL *row = ll_new(NULL, NULL);

        for (int x = 0; x < w; ++x) {
            Vec2 v = (Vec2){
                .x = x,
                .y = y
            };

            ll_add(row, &v, sizeof(Vec2));
        }

        ll_add(positions, row, 0);
    }

    LLIter iter = ll_iter(positions);

    while (ll_iter_has(&iter)) {
        LLIterItem item = ll_iter_consume(&iter);

        LL *row = item.data;

        LLIter iter = ll_iter(row);

        while (ll_iter_has(&iter)) {
            LLIterItem item = ll_iter_consume(&iter);
            Vec2 pos = *(Vec2*)item.data;

            printf("(%02d, %02d) ", pos.x, pos.y);
        }

        printf("\n");
    }

    ll_free(positions);

    return 0;
}
