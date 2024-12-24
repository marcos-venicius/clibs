#include "ll.h"
#include "assertf.h"

int main() {
    LL *list = ll_new(NULL, NULL);

    int t;
    int a = 0;
    int b = 1;

    for (int i = 0; i < 20; ++i) {
        ll_add_i(list, a);

        t = a;
        a = b;
        b = t + b;
    }

    LLIter iter = ll_iter(list);

    while (ll_iter_has(&iter)) {
        LLIterItem item = ll_iter_consume(&iter);

        printf("%02ld %d\n", item.index, *(int*)item.data);
    }

    ll_iter_flush(&iter);

    while (ll_iter_has(&iter)) {
        LLIterItem item = ll_iter_consume(&iter);

        printf("%02ld %d\n", item.index, *(int*)item.data);
    }

    ll_free(list);

    return 0;
}
