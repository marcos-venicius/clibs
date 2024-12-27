# CLIBS

_**Usefull libraries in C** based on my needs_.

## LL

Extends for (Linked List).

This DS works like an array.

```c
LL *ll_new(FreeCallback free_callback, CompareCallback compare_callback);
```

Create a new instance of `LL`.

---

```c
void ll_add(LL *ll, void *data, size_t data_size);
```

Add any data of any size to the linked list (ideally a Linked List should have only one data type).

To add a `LL*` as value you can just do something like this:

```c
    ...

    LL *positions = ll_new((FreeCallback)ll_free, NULL);

    ...

    LL *row = ll_new(NULL, NULL);

    ll_add_i(row, 10);

    ll_add(positions, row, 0); // it's important the size being 0

    ll_free(positions);
```

You don't need to free the row, the parent list will do this job for you!

---

```c
bool int_compare(void *a, void *b);
```

A built-in int comparer.

---

```c
bool string_compare(void *a, void *b);
```

A built-in string comparer.

---

```c
void ll_add_i(LL *ll, int i);
```

Make the process of adding an integer to the list easy.

---

```c
void ll_add_s(LL *ll, char *s);
```

Make the process of adding a string to the list easy.

---

```c
void ll_remove_by_value(LL *ll, void *data);
```

Allows you to remove the first item that matches the data value you pass as argument.

**It's important to notice that to remove by value you should pass a `CompareCallback` in `ll_new`**.

```c
void ll_remove_by_value_i(LL *ll, int i);
```

Makes removing an integer easy.

---

```c
void ll_remove_by_index(LL *ll, size_t index);
```

Allows you to remove a value by index.

---

```c
void *ll_find_by_value(LL *ll, void *data);
```

Allows you to get an item by your value.

**It's important to notice that to find by value you should pass a `CompareCallback` in `ll_new`**.

---

```c
void *ll_find_by_index(LL *ll, size_t index);
```

Allows you to get an item by your index.

---

```c
LLIter ll_iter(LL *ll);
```

Create a new iterator to allow you easely iterate over your list.

Example:

```c
    LL *list = ll_new(NULL, NULL);

    LLIter iter = ll_iter(list);

    while (ll_iter_has(&iter)) {
        LLIterItem item = ll_iter_consume(&iter);

        printf("%ld\n", item.index);
    }

    ll_iter_flush(&iter);
```

---

```c
bool ll_iter_has(LLIter *iter);
```

Check if the iterator is not empty.

---

```c
LLIterItem ll_iter_consume(LLIter *iter);
```

Consumes the current item of the iterator.

---

```c
void ll_iter_flush(LLIter *iter);
```

Allows your to restore the state of the iterator to be able to iterate again.

If you don't call this function before using it again, it'll crash your program with a message.

**You need to call this flush after all modifications in the list** if you don't do this, it's possible to get wrong addresses after removing it.

---

```c
void ll_free(LL *ll);
```

Free the instance.

## Map

**WIP**
