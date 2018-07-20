#pragma once

typedef enum {
    // Single-character tokens.
    TOKEN_COLON, TOKEN_COMMA,

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_NUMBER,

    // Keywords.
    #define INSTRUCTION(name, length)   TOKEN_##name,
    #include "instruction.h"
    #undef INSTRUCTION

    // Special purpose tokens.
    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

void initScanner(const char* source);
Token scanToken();
