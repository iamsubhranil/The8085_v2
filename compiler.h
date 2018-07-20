#pragma once

#include "common.h"

typedef enum{
    PARSE_ERROR,
    MEMORY_FULL,
    LABEL_FULL,
    LABELS_PENDING,
    COMPILE_OK
} CompilationStatus;

// Compiler will halt in the first error
CompilationStatus compile(const char *source, u8 *memory, u16 size, u16 *offset);
