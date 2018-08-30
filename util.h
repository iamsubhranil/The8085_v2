#pragma once

#include <stdbool.h>

#include "common.h"

char* readFile(const char *path);
bool parse_hex_byte(const char *str, u8 *store);
bool parse_hex_16(const char *str, u16 *store);
