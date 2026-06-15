#include <stdbool.h>
#include <string.h>

#include "./utils.h"

bool cmp_const_sized_str(const char *conzt, const char *sized, int length) {
    int conzt_size = strlen(conzt);

    if (length != conzt_size) return false;

    return strncmp(conzt, sized, length) == 0;
}
