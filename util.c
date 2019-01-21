#include <stdio.h>

#include "common.h"
#include "display.h"
#include "util.h"

char *readFile(const char *path) {
	FILE *file = fopen(path, "rb");

	if(file == NULL) {
		perr("Could not open file \"%s\".\n", path);
		return NULL;
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	char *buffer = (char *)malloc(fileSize + 1);
	if(buffer == NULL) {
		perr("Not enough memory to read \"%s\".\n", path);
		return NULL;
	}

	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
	if(bytesRead < fileSize) {
		perr("Could not read file \"%s\".\n", path);
		return NULL;
	}
	buffer[bytesRead] = '\0';

	fclose(file);
	return buffer;
}

bool parse_hex_byte(const char *str, u8 *store) {
	char *end = NULL;
	u32   val;
	val = strtol(str, &end, 16);
	if(*end != 0) {
		perr("Bad byte '%s'!", str);
		return 0;
	}
	if(val > 0xff) {
		perr("Byte out of range : '%s'!", str);
		return 0;
	}
	*store = val;
	return 1;
}

bool parse_hex_16(const char *str, u16 *store) {
	char *end = NULL;
	u32   addr;
	addr = strtol(str, &end, 16);
	if(*end != 0) {
		perr("Bad address '%s'!", str);
		return 0;
	}
	if(addr > 0xffff) {
		perr("Address out of range : '%s'!", str);
		return 0;
	}
	*store = addr;
	return 1;
}

// clang-format off

int get_string_index(Keyword *keywords, siz numKeywords, const char *string, siz length) {
    siz start = 0, end = 0;
    // Find the initial boundary
    while(start < numKeywords &&                            // the array is not out of bounds       and
            (string[0] > keywords[start].str[0]             // ( there are still letters to come      or
             || (string[0] == keywords[start].str[0]        //    ( this is the required letter          and
                 && length != keywords[start].length))) {   //      the length doesn't match))
        start++;
    }
    if(start == numKeywords                                 // array exhausted          or 
            || string[0] != keywords[start].str[0])         // this is not the initial letter that was searched
        return -1;
    end = start;
    // Find the terminate boundary
    while(end < numKeywords                                 // the array is not out of bounds       and 
            && keywords[end].str[0] == string[0]            // the letters match                    and
            /*&& keywords[end].length == length*/)          // the lengths match
        end++;

    siz temp = start, matching = 1;
    while(temp < end && matching < length) {                    // the search is in boundary and not all letters have been checked
        if(keywords[temp].str[matching] == string[matching])    // if: present letter matches
            matching++;                                         //      then: check for the next letter
        else if(keywords[temp].str[matching] > string[matching])// else if: present letter of 'string' is lesser than that of 'keyword'
            break;                                              //      then: since 'keywords' is a sorted collection,
                                                                //            'string' will not be found later, bail out
        else{                                                   // else: present letter doesn't match
            temp++;                                             //      so: check for the next word
            while(temp < end && keywords[temp].length != length)
                temp++;
        }
    }

    if(matching == length)                                  // all letters have matched
        return temp;

    return -1;
}

// clang-format on

// Assumes 0 <= max <= RAND_MAX
// Returns in the closed interval [0, max]
i64 random_at_most(i64 max) {
	i64
	    // max <= RAND_MAX < ULONG_MAX, so this is okay.
	    num_bins = max + 1,
	    num_rand = (i64)RAND_MAX + 1, bin_size = num_rand / num_bins,
	    defect = num_rand % num_bins;

	i64 x;
	do {
		x = random();
	}
	// This is carefully written not to overflow
	while(num_rand - defect <= (i64)x);

	// Truncated division is intentional
	return x / bin_size;
}
