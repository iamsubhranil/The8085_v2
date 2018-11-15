#include <stdio.h>
#include <string.h>

#include "common.h"
#include "display.h"
#include "scanner.h"
#include "util.h"

typedef struct {
	const char *start;
	const char *current;
	const char *source;
	int         line;
} Scanner;

Scanner scanner;

void initScanner(const char *source) {
	scanner.start   = source;
	scanner.current = source;
	scanner.source  = source;
	scanner.line    = 1;
}

static bool isAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	       (c >= 'A' && c <= 'F');
}

static bool isAtEnd() {
	return *scanner.current == '\0';
}

static char advance() {
	scanner.current++;
	return scanner.current[-1];
}

static char peek() {
	return *scanner.current;
}

static char peekNext() {
	if(isAtEnd())
		return '\0';
	return scanner.current[1];
}

static Token makeToken(TokenType type) {
	Token token;
	token.type   = type;
	token.start  = scanner.start;
	token.length = (int)(scanner.current - scanner.start);
	token.line   = scanner.line;
	token.chidx  = scanner.start - scanner.source;
	return token;
}

static Token errorToken() {
	return makeToken(TOKEN_ERROR);
}

void token_highlight_source(Token t) {
	int         line = 1;
	const char *s    = scanner.source;
	while(line < t.line) {
		if(*s == '\n')
			line++;
		s++;
		t.chidx--;
	}
	pgrn(ANSI_FONT_BOLD "\n[line %d] " ANSI_COLOR_RESET, line);
	while(*s != '\n' && *s != '\0') {
		putchar(*s);
		s++;
	}
	printf("\n        %.*s", line > 9 ? 2 : line > 99 ? 3 : 1, "    ");
	while(t.chidx > 0) {
		printf(" ");
		t.chidx--;
	}
	while(t.length > 0) {
		pylw(ANSI_FONT_BOLD "^" ANSI_COLOR_RESET);
		t.length--;
	}
}

static void skipWhitespace() {
	for(;;) {
		char c = peek();
		switch(c) {
			case ' ':
			case '\r':
			case '\t': advance(); break;

			case '\n':
				scanner.line++;
				advance();
				break;

			case '/':
				if(peekNext() == '/') {
					// A comment goes until the end of the line.
					while(peek() != '\n' && !isAtEnd()) advance();
				} else {
					return;
				}
				break;

			default: return;
		}
	}
}

static TokenType keyword_types[] = {
#define INSTRUCTION(name, length) TOKEN_##name,
#include "instruction.h"
#undef INSTRUCTION
};

Keyword instruction_keywords[] = {
#define INSTRUCTION(name, length) {#name, length},
#include "instruction.h"
#undef INSTRUCTION
};

siz instruction_keywords_count = sizeof(instruction_keywords) / sizeof(Keyword);

static TokenType identifierType() {
	int idx = get_string_index(instruction_keywords, instruction_keywords_count,
	                           scanner.start, scanner.current - scanner.start);
	if(idx == -1)
		return TOKEN_IDENTIFIER;
	return keyword_types[idx];
}

static Token identifier() {
	while(isAlpha(peek()) || isDigit(peek())) advance();

	return makeToken(identifierType());
}

static Token number() {
	while(isDigit(peek())) advance();

	return makeToken(TOKEN_NUMBER);
}

Token scanToken() {
	skipWhitespace();

	scanner.start = scanner.current;

	if(isAtEnd())
		return makeToken(TOKEN_EOF);

	char c = advance();

	if(isAlpha(c))
		return identifier();
	if(isDigit(c))
		return number();

	switch(c) {
		case ':': return makeToken(TOKEN_COLON);
		case ',': return makeToken(TOKEN_COMMA);
	}

	return errorToken();
}
