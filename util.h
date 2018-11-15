#pragma once

#include <stdbool.h>

#include "common.h"

char *readFile(const char *path);
bool  parse_hex_byte(const char *str, u8 *store);
bool  parse_hex_16(const char *str, u16 *store);
i64   random_at_most(i64 n);

// Since it is likely that the search function
// will be called multiple times, it is efficient
// and better to store the search items and their
// length in a non-volatile data structure, for
// better reusability. Also, it is highly likely
// the search dictionary will be populated compile
// time, and hence, calculating the length
// of the strings will incur no additional
// overhead to the search function at all, even
// for a sufficiently large dictionary.
typedef struct {
	const char *str;
	siz         length;
} Keyword;
// Searches for 'str_to_search' in the given arr
// If any match is found, then its index in the
// arr is returned. Otherwise -1 is returned (that's
// the only reason of the type being 'int')
int get_string_index(Keyword *arr, siz arr_length, const char *str_to_search,
                     siz str_length);
