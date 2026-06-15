#ifndef __cearch_utils_h_
#define __cearch_utils_h_

typedef struct {
    int line, col_start, col_end;
} cearch_location_t;

typedef struct {
    const char *value;
    int size;
} cearch_string_t;

bool cmp_const_sized_str(const char *conzt, const char *sized, int length);

#endif // __cearch_utils_h_
