#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

static bool isDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
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
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;

    scanner.current++;
    return true;
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;

    return token;
}

static Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;

    return token;
}

static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;

            case '\n':
                scanner.line++;
                advance();
                break;

            case '/':
                if (peekNext() == '/') {
                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return;
                }
                break;

            default:
                return;
        }
    }
}

static TokenType checkKeyword(int start, int length,
        const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
            memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

typedef struct{
    const char *str;
    siz length;
    TokenType token;
} Keyword;

static Keyword keywords[] = {
    {"add", 3, TOKEN_ADD},
    {"addi", 4, TOKEN_ADDI},
    {"ana", 3, TOKEN_ANA},
    {"ani", 3, TOKEN_ANI},
    {"call", 4, TOKEN_CALL},
    {"cc", 2, TOKEN_CC},
    {"cm", 2, TOKEN_CM},
    {"cma", 3, TOKEN_CMA},
    {"cmp", 3, TOKEN_CMP},
    {"cnc", 3, TOKEN_CNC},
    {"cnz", 3, TOKEN_CNZ},
    {"cp", 2, TOKEN_CP},
    {"cpi", 3, TOKEN_CPI},
    {"cz", 2, TOKEN_CZ},
    {"dcr", 3, TOKEN_DCR},
    {"dcx", 3, TOKEN_DCX},
    {"hlt", 3, TOKEN_HLT},
    {"in", 2, TOKEN_IN},
    {"inr", 3, TOKEN_INR},
    {"inx", 3, TOKEN_INX},
    {"jc", 2, TOKEN_JC},
    {"jm", 2, TOKEN_JM},
    {"jmp", 3, TOKEN_JMP},
    {"jnc", 3, TOKEN_JNC},
    {"jnz", 3, TOKEN_JNZ},
    {"jp", 2, TOKEN_JP},
    {"jz", 2, TOKEN_JZ},
    {"lda", 3, TOKEN_LDA},
    {"ldax", 4, TOKEN_LDAX},
    {"lxi", 3, TOKEN_LXI},
    {"mov", 3, TOKEN_MOV},
    {"mvi", 3, TOKEN_MVI},
    {"nop", 3, TOKEN_NOP},
    {"ora", 3, TOKEN_ORA},
    {"ori", 3, TOKEN_ORI},
    {"out", 3, TOKEN_OUT},
    {"ral", 3, TOKEN_RAL},
    {"rar", 3, TOKEN_RAR},
    {"rc", 2, TOKEN_RC},
    {"ret", 3, TOKEN_RET},
    {"rlc", 3, TOKEN_RLC},
    {"rm", 2, TOKEN_RM},
    {"rnc", 3, TOKEN_RNC},
    {"rnz", 3, TOKEN_RNZ},
    {"rp", 2, TOKEN_RP},
    {"rrc", 3, TOKEN_RRC},
    {"rz", 2, TOKEN_RZ},
    {"sta", 3, TOKEN_STA},
    {"stax", 4, TOKEN_STAX},
    {"sub", 3, TOKEN_SUB},
    {"sui", 3, TOKEN_SUI},
    {"xra", 3, TOKEN_XRA},
    {"xri", 3, TOKEN_XRI}
};

static siz numKeywords = sizeof(keywords)/sizeof(Keyword);

static TokenType identifierType(){
    u64 length = scanner.current - scanner.start;
    u64 start = 0, end = 0;
    // Find the initial boundary
    while(start < numKeywords &&                            // the array is not out of bounds       and
            (scanner.start[0] > keywords[start].str[0]      // ( there are still letters to come      or
             || (scanner.start[0] == keywords[start].str[0] //    ( this is the required letter          and
                 && length != keywords[start].length))){    //      the length doesn't match))
        start++;
    }
    if(start == numKeywords                                 // array exhausted          or 
            || scanner.start[0] != keywords[start].str[0])  // this is not the initial letter that was searched
        return TOKEN_IDENTIFIER;
    end = start;
    // Find the terminate boundary
    while(end < numKeywords                                 // the array is not out of bounds       and 
            && keywords[end].str[0] == scanner.start[0]     // the letters match                    and
            /*&& keywords[end].length == length*/)              // the lengths match
        end++;

    u64 temp = start, matching = 1;
    while(temp < end && matching < length){                 // the search is in boundary and not all letters have been checked
        if(keywords[temp].str[matching] == scanner.start[matching])     // present letter matches
            matching++;                                                 // so check for the next letter
        else{                                                            // present letter doesn't match
            temp++;                                                     // so check for the next word
            while(temp < end && keywords[temp].length != length)
                temp++;
        }
    }

    if(matching == length)                                  // all letters have matched
        return keywords[temp].token;

    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();

    return makeToken(identifierType());
}

static Token number() {
    while (isDigit(peek())) advance();

    return makeToken(TOKEN_NUMBER);
}

Token scanToken() {
    skipWhitespace();

    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();

    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
        case ':': return makeToken(TOKEN_COLON);
        case ',': return makeToken(TOKEN_COMMA);
    }

    return errorToken("Unexpected character.");
}
