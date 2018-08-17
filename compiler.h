#pragma once

#include "common.h"

typedef enum{
    PARSE_ERROR,
    MEMORY_FULL,
    LABEL_FULL,
    LABELS_PENDING,
    COMPILE_OK
} CompilationStatus;

// Reset the internal states of the compiler
void compiler_reset();
// Compiler will halt in the first error
CompilationStatus compile(const char *source, u8 *memory, u16 size, u16 *offset);
