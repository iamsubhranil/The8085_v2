#pragma once

#include "common.h"

typedef enum{
    PARSE_ERROR,
    MEMORY_FULL,
    LABEL_FULL,
    LABELS_PENDING,
    EMPTY_PROGRAM,
    NO_HLT,
    COMPILE_OK
} CompilationStatus;

// Write a byte to the active chunk
// To be called from codegen_*.c
u16 compiler_write_byte(u8 byte);
// Reset the internal states of the compiler
void compiler_reset();
// Compiler will halt in the first error
CompilationStatus compile(const char *source, u8 *memory, u16 size, u16 *offset);
// Check for and report the presence of pending labels
void compiler_report_pending();
