#pragma once

#include "util.h"

typedef enum {
	// Single-character tokens.
	TOKEN_COLON,
	TOKEN_COMMA,

	// Literals.
	TOKEN_IDENTIFIER,
	TOKEN_NUMBER,

// Keywords.
#define INSTRUCTION(name, length) TOKEN_##name,
#include "instruction.h"
#undef INSTRUCTION

	// Special purpose tokens.
	TOKEN_ERROR,
	TOKEN_EOF
} TokenType;

typedef struct {
	TokenType   type;
	const char *start;
	int         length;
	int         line;
	int         chidx;
} Token;

// Globally accessible keyword dictionary
extern Keyword instruction_keywords[];
extern siz     instruction_keywords_count;

void  initScanner(const char *source);
Token scanToken();
void  token_highlight_source(Token t);
