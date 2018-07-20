#pragma once

typedef enum {
    // Single-character tokens.
    TOKEN_COLON, TOKEN_COMMA,

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_NUMBER,

    // Keywords.
    TOKEN_MVI, TOKEN_MOV, TOKEN_LXI, TOKEN_LDA,
    TOKEN_STA, TOKEN_LDAX, TOKEN_STAX, TOKEN_OUT,
    TOKEN_IN,

    TOKEN_ADD, TOKEN_ADDI, TOKEN_SUB, TOKEN_SUI,
    TOKEN_INR, TOKEN_INX, TOKEN_DCR, TOKEN_DCX,

    TOKEN_ANA, TOKEN_ANI, TOKEN_ORA, TOKEN_ORI,
    TOKEN_XRA, TOKEN_XRI, TOKEN_CMP, TOKEN_CPI,
    TOKEN_CMA, TOKEN_RLC, TOKEN_RAL, TOKEN_RRC,
    TOKEN_RAR,

    TOKEN_JMP, TOKEN_JC, TOKEN_JNC, TOKEN_JZ,
    TOKEN_JNZ, TOKEN_JP, TOKEN_JM, 
    
    TOKEN_CALL, TOKEN_CC, TOKEN_CNC, TOKEN_CZ, 
    TOKEN_CNZ, TOKEN_CP, TOKEN_CM, 
    
    TOKEN_RET, TOKEN_RC, TOKEN_RNC,
    TOKEN_RZ, TOKEN_RNZ, TOKEN_RP, TOKEN_RM,

    TOKEN_HLT, TOKEN_NOP,

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
