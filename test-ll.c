#include "ll.h"
#include "assertf.h"

int main() {
    LL *list = ll_new(NULL, int_compare);

    ll_add_i(list, 100);
    // test remove first/last value
    ll_remove_by_index(list, 0);

    assertf(list->count == 0, "list should be empty");

    ll_add_i(list, 100);

    // test remove first/last element by value
    ll_remove_by_value_i(list, 100);

    assertf(list->count == 0, "list should be empty");
    assertf(list->head == NULL, "head should be null");
    assertf(list->tail == NULL, "tail should be null");

    for (int i = 1; i <= 20; ++i) {
        ll_add_i(list, i);
    }

    // test remove first element
    ll_remove_by_index(list, 0);

    // test remove middle element
    ll_remove_by_index(list, list->count / 2);

    // test remove last element
    ll_remove_by_index(list, list->count - 1);

    LLIter iter = ll_iter(list);

    while (ll_iter_has(&iter)) {
        LLIterItem item = ll_iter_consume(&iter);

        printf("%02ld %d\n", item.index, *(int*)item.data);
    }

    printf("---------------------\n");
    // test search first digit
    int firstDigit = *(int*)ll_find_by_index(list, 0);
    int middleDigit = *(int*)ll_find_by_index(list, list->count / 2);
    int lastDigit = *(int*)ll_find_by_index(list, list->count - 1);

    printf("first digit: %d\n", firstDigit);
    printf("middle digit: %d\n", middleDigit);
    printf("last digit: %d\n", lastDigit);

    // test remove by value first digit
    ll_remove_by_value(list, &firstDigit);
    // test remove by value middle digit
    ll_remove_by_value(list, &middleDigit);
    // test remove by value last digit
    ll_remove_by_value(list, &lastDigit);

    // test flush
    ll_iter_flush(&iter);

    while (ll_iter_has(&iter)) {
        LLIterItem item = ll_iter_consume(&iter);

        printf("%02ld %d\n", item.index, *(int*)item.data);
    }

    ll_free(list);

    return 0;
}
