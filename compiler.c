#include <memory.h>

#include "bytecode.h"
#include "codegen.h"
#include "compiler.h"
#include "display.h"
#include "scanner.h"
#include "vm.h"

// We statically allocate labels to reduce overhead
#define NUM_LABELS  32

typedef struct{
    char *label;        // name of the label
    int length;         // length of the string
    u16 offset;         // the offset at which the label is declared
    u8 isDeclared;      // marker to denote if the label is declared
} Label;

// Statically allocate pending labels too
#define NUM_PENDING_LABELS  32
typedef struct{
    Token token;        // Cached token for error reporting
    u16 idx;            // If the label was already found in the labelTable, this denotes its index
                        // otherwise it is the last but oneth index, i.e. the last index of the table
                        // in the moment it was used
    u16 offset;         // offset in the memory where the label is used, to patch later by the address
                        // of the label
    u8 decl_shown;      // Denotes whether the warning of using this undeclared label was already
                        // shown
} PendingLabel;

// C style list of labels
static Label labelTable[NUM_LABELS] = {{NULL, 0, 0, 0}};
static siz labelPointer = 0;

// C style list of pending labels
static PendingLabel pending_labels[NUM_PENDING_LABELS] = {{{}, 0, 0, 0}};
static siz pendingPointer = 0;

// Memory management for actually writing bytes
static u16 memSize = 0, *offset = NULL;
static u8 *memory = NULL;

// Since compiler_write_byte cannot directly return an
// error code, it will denote memory full
// by triggering this
static u8 memory_full = 0;

// Consumed tokens
static Token presentToken = {}, previousToken = {};

// Function type which compiles a particular token.
// Since we're writing more of an assembler of sort,
// each sentence will either begin with an opcode,
// or a label declaration. Based on the scanned token
// type, from the compilationTable, we can easily
// decide and invoke compilation action for the 
// specific token
typedef CompilationStatus (*compilerFn)(Token t);

// Write a byte to the memory and manage the offset
u16 compiler_write_byte(u8 value){
    if(memory_full || *offset >= memSize){
        memory_full = 1;
        return *offset;
    }
    memory[*offset] = value;
    (*offset)++;
    return (*offset) - 1;
}

// Write two bytes to the memory
u16 write_dword(u16 value){
    compiler_write_byte(value & 0x00ff);
    compiler_write_byte((value & 0xff00) >> 8);
    return (*offset) - 2;
}

// The typical message to be shown when an operand
// is of an unexpected type
static void unexpected_operand(const char *expected, Token op, Token t){
    perr("Expected %s after " ANSI_FONT_BOLD "%.*s" ANSI_COLOR_RESET "!",
            expected, op.length, op.start);
    token_highlight_source(t);
}

// Proceed to the next token.
// We have no token list of any sort. We
// are basically just scanning on demand
// and compiling.
// This reduces a lot of malloc's and
// the amount of total work when an error
// has occurred, since, the remaining portion
// of the input will not even be scanned.
static Token advance(){
    if(presentToken.type == TOKEN_EOF)
        return presentToken;
    previousToken = presentToken;
    presentToken = scanToken();
    return presentToken;
}

// Check if the next token is of the expected
// type. If it is, then silently consume it.
// Otherwise, show an error message.
static bool consume(TokenType type, const char *message){
    if(advance().type != type){
        perr("%s", message);
        token_highlight_source(presentToken);
        return false;
    }
    return true;
}

// Check whether the given token is a valid
// register
static bool isreg(Token t){
    return t.length == 1 && 
        ((t.start[0] >= 'a' && t.start[0] <= 'e') || t.start[0] == 'h' || t.start[0] == 'l');
}

// Check whether the given token is 'm'
static bool ismem(Token t){
    return t.length == 1 && t.start[0] == 'm';
}

// Check whether the given token is a valid
// register pair
static bool isregpair(Token t){
    return t.length == 1 && (t.start[0] == 'b' || t.start[0] == 'd' || t.start[0] == 'h');
}

// Check whether the given token is 'sp'
static bool issp(Token t){
    return t.length == 2 && t.start[0] == 's' && t.start[1] == 'p';
}

// Check whether the given token is 'psw'
static bool ispsw(Token t){
    return t.type == TOKEN_IDENTIFIER && t.length == 3
        && t.start[0] == 'p' && t.start[1] == 's' && t.start[2] == 'w';
} 

// This is eventually the compilerFn invoked
// when the start token of a statement
// isn't what we expect it to be
CompilationStatus compile_unexpected_token(Token t){
    perr("Unexpected token at line %d!", t.line);
    token_highlight_source(t);
    return PARSE_ERROR;
}

// Compiles a label and adds it to the labelTable
CompilationStatus compile_label(Token t){
    advance();
    if(presentToken.type != TOKEN_COLON){
        perr("Expected ':' after '%.*s', received '%.*s'!", 
                t.length, t.start,
                presentToken.length, presentToken.start);
        token_highlight_source(t);
        token_highlight_source(presentToken);
        return PARSE_ERROR;
    }
    if(labelPointer == NUM_LABELS)
        return LABEL_FULL;
    for(u16 i = 0;i < labelPointer;i++){
        if(labelTable[i].length == t.length
                && memcmp(labelTable[i].label, t.start, t.length) == 0){
            labelTable[i].offset = *offset;
            labelTable[i].isDeclared = 1;
            return COMPILE_OK;
        }
    }
    labelTable[labelPointer].label = strdup(t.start);
    labelTable[labelPointer].length = t.length;
    labelTable[labelPointer].offset = *offset;
    labelTable[labelPointer].isDeclared = 1;
    labelPointer++;
    return COMPILE_OK;
}

// Compiles a hex operand - both 8 and 16 bit.
// Since labels can also be used as memory
// address, we also consider any identifier
// to be a potential 16bit value, which
// will be the address of declaration of
// the label, if found.
CompilationStatus compile_hex(u8 is16){
    Token t = advance();
    if(t.type != TOKEN_NUMBER){
        if(t.type == TOKEN_IDENTIFIER && is16 == 1){
            // Try to patch it immediately
            u8 found = 0;
            u16 idx = 0;
            for(u16 i = 0;i < labelPointer;i++){
                if(labelTable[i].length == t.length
                        && !memcmp(labelTable[i].label, t.start, t.length)){
                    if(labelTable[i].isDeclared){
                        write_dword(labelTable[i].offset);
                        return COMPILE_OK;
                    }
                    else{
                        found = 1;  // The label was found, but it is not declared yet
                        idx = i;
                        break;
                    }
                }
            }
            // It might be a forward reference
            if(!found){ // The label was not found earlier
                labelTable[labelPointer].label = strdup(t.start);
                labelTable[labelPointer].length = t.length;
                labelTable[labelPointer].offset = 0;
                labelTable[labelPointer].isDeclared = 0;
                labelPointer++;
            }

            pending_labels[pendingPointer].idx = found ? idx : (labelPointer - 1);
            pending_labels[pendingPointer].offset = *offset;
            pending_labels[pendingPointer].token = t;
            pending_labels[pendingPointer].token.start = strdup(t.start);
            pendingPointer++;
            write_dword(0);
            return COMPILE_OK;
        }
        perr("Expected %d bit number!", (8*(is16 + 1)));
        token_highlight_source(t);
        return PARSE_ERROR;
    }
    advance();
    if(presentToken.type != TOKEN_IDENTIFIER){
        perr("Expected 'h' after number!");
        token_highlight_source(previousToken);
        return PARSE_ERROR;
    }
    if(presentToken.length != 1 || presentToken.start[0] != 'h'){
        perr("Expected 'h' after number!");
        token_highlight_source(previousToken);
        return PARSE_ERROR;
    }
    
    u32 number = 0;
    for(int i = previousToken.length - 1;i >= 0;i--){
        char c = previousToken.start[i];
        u32 n = (c >= 'a' ? c - 'a' + 10 : c - '0');
        number |= n << ((previousToken.length - i - 1) * 4);
    }

    u32 range = is16 ? 0xffff : 0x00ff;
    
    if(number > range){
        perr("Hex number out of range! [should be < 0x%x]", range);
        token_highlight_source(previousToken);
        return PARSE_ERROR;
    }
    if(is16)
        write_dword(number);
    else
        compiler_write_byte(number & 0x00ff);
    return COMPILE_OK;
}

// Compile an instruction with no operand
static CompilationStatus compile_no_operand(Token t){
    codegen_no_op(t);
    return COMPILE_OK;
}

// Compile an instruction with one 8 bit
// immediate operand
static CompilationStatus compile_hex8_operand(Token t){
    codegen_no_op(t);
    return compile_hex(0);
}

// Compile an instruction with one 16 bit
// immediate operand
static CompilationStatus compile_hex16_operand(Token t){
    codegen_no_op(t);
    return compile_hex(1);
}

// Compile an instruction of type
// opcode [r/m]
static CompilationStatus compile_reg_or_mem(Token t){
    if(isreg(advance())){ // check whether or not the next token is a register
        codegen_reg(t, presentToken);
    }
    else if(ismem(presentToken)){
        codegen_mem(t);
    }
    else{
        unexpected_operand("register or memory", t, presentToken);
        return PARSE_ERROR;
    }
    return COMPILE_OK;
}

// Compile an instruction of type
// opcode [regpair/sp]
static CompilationStatus compile_regpair_or_sp(Token t){
    if(isregpair(advance())){ // check whether or not the next token is a register pair
        codegen_regpair(t, presentToken);
    }
    else if(issp(presentToken)){
        codegen_sp(t);
    }
    else{
        unexpected_operand("register pair or stack pointer", t, presentToken);
        return PARSE_ERROR;
    }
    return COMPILE_OK;
}

// Compile an instruction of type
// opcode [regpair/psw]
static CompilationStatus compile_regpair_or_psw(Token t){
    if(isregpair(advance())){ // check whether or not the next token is a register pair
        codegen_regpair(t, presentToken);
    }
    else if(ispsw(presentToken)){
        codegen_psw(t);
    }
    else{
        unexpected_operand("register pair or program status word", t, presentToken);
        return PARSE_ERROR;
    }
    return COMPILE_OK;
}

// Some special compilation methods
// for some specific instructions, usually
// the instructions with more than one
// operand

static CompilationStatus compile_mvi(Token t){
    CompilationStatus st = COMPILE_OK;
    if((st = compile_reg_or_mem(t)) != COMPILE_OK)
        return st;
    if(!consume(TOKEN_COMMA, "Expected comma between operands"))
        return PARSE_ERROR;
    return compile_hex(0);
}

static CompilationStatus compile_lxi(Token t){
    CompilationStatus st = COMPILE_OK;
    if((st = compile_regpair_or_sp(t)) != COMPILE_OK)
        return st;
    if(!consume(TOKEN_COMMA, "Expected comma between operands!"))
        return PARSE_ERROR;
    return compile_hex(1);
}

static CompilationStatus compile_mov(Token t){
    if(isreg(advance())){
        Token prevreg = presentToken;
        if(!consume(TOKEN_COMMA, "Expected comma between operands!"))
            return PARSE_ERROR;
        if(isreg(advance())){
            codegen_mov_r_r(prevreg, presentToken);
            return COMPILE_OK;
        }
        else if(ismem(presentToken)){
            codegen_mov_r_m(prevreg);
            return COMPILE_OK;
        }
        else{
            perr("Expected register or memory!"); 
            token_highlight_source(presentToken);
            return PARSE_ERROR;
        }
    }
    else if(ismem(presentToken)){
        if(!consume(TOKEN_COMMA, "Expected comma between operands!"))
            return PARSE_ERROR;
        if(isreg(advance())){
            codegen_mov_m_r(presentToken);
            return COMPILE_OK;
        }
        else{
            perr("Expected register!"); 
            token_highlight_source(presentToken);
            return PARSE_ERROR;
        }
    }
    else{
        unexpected_operand("register or memory", t, presentToken);
        return PARSE_ERROR;
    }
}

static CompilationStatus compile_ldax(Token t){
    if(isregpair(advance()) && 
            (presentToken.start[0] == 'b'
             || presentToken.start[0] == 'd')){
            codegen_regpair(t, presentToken);
            return COMPILE_OK; 
    }
    else{
        unexpected_operand("register pair 'b' or 'd'", t, presentToken);
        return PARSE_ERROR;
    }
}

// Table which maps the compilation
// function to each token type.
// Basically this is the table which
// denotes what function to invoke
// for an opcode to compile.
static compilerFn compilationTable[] = {
    // Single-character tokens.
    compile_unexpected_token,   // TOKEN_COLON
    compile_unexpected_token,   // TOKEN_COMMA

    // Literals.
    compile_label,              // TOKEN_IDENTIFIER
    compile_unexpected_token,   // TOKEN_NUMBER

    // Keywords.
    compile_hex8_operand,       // TOKEN_ACI
    compile_reg_or_mem,         // TOKEN_ADC
    compile_reg_or_mem,         // TOKEN_ADD
    compile_hex8_operand,       // TOKEN_ADI
    compile_reg_or_mem,         // TOKEN_ANA
    compile_hex8_operand,       // TOKEN_ANI
    
    compile_hex16_operand,      // TOKEN_CALL
    compile_hex16_operand,      // TOKEN_CC
    compile_hex16_operand,      // TOKEN_CM
    compile_no_operand,         // TOKEN_CMA
    compile_no_operand,         // TOKEN_CMC
    compile_reg_or_mem,         // TOKEN_CMP
    compile_hex16_operand,      // TOKEN_CNC
    compile_hex16_operand,      // TOKEN_CNZ
    compile_hex16_operand,      // TOKEN_CP
    compile_hex16_operand,      // TOKEN_CPE
    compile_hex8_operand,       // TOKEN_CPI
    compile_hex16_operand,      // TOKEN_CPO
    compile_hex16_operand,      // TOKEN_CZ

    compile_no_operand,         // TOKEN_DAA
    compile_regpair_or_sp,      // TOKEN_DAD
    compile_reg_or_mem,         // TOKEN_DCR
    compile_regpair_or_sp,      // TOKEN_DCX

    compile_no_operand,         // TOKEN_HLT
    
    compile_hex8_operand,       // TOKEN_IN
    compile_reg_or_mem,         // TOKEN_INR
    compile_regpair_or_sp,      // TOKEN_INX
 
    compile_hex16_operand,      // TOKEN_JC
    compile_hex16_operand,      // TOKEN_JM
    compile_hex16_operand,      // TOKEN_JMP
    compile_hex16_operand,      // TOKEN_JNC
    compile_hex16_operand,      // TOKEN_JNZ
    compile_hex16_operand,      // TOKEN_JP
    compile_hex16_operand,      // TOKEN_JPE
    compile_hex16_operand,      // TOKEN_JPO
    compile_hex16_operand,      // TOKEN_JZ   
    
    compile_hex16_operand,      // TOKEN_LDA
    compile_ldax,               // TOKEN_LDAX
    compile_hex16_operand,      // TOKEN_LHLD
    compile_lxi,                // TOKEN_LXI
    
    compile_mov,                // TOKEN_MOV
    compile_mvi,                // TOKEN_MVI
    
    compile_no_operand,         // TOKEN_NOP 
 
    compile_reg_or_mem,         // TOKEN_ORA
    compile_hex8_operand,       // TOKEN_ORI
    compile_hex8_operand,       // TOKEN_OUT   
   
    compile_no_operand,         // TOKEN_PCHL
    compile_regpair_or_psw,     // TOKEN_POP
    compile_regpair_or_psw,     // TOKEN_PUSH

    compile_no_operand,         // TOKEN_RAL
    compile_no_operand,         // TOKEN_RAR
    compile_no_operand,         // TOKEN_RC
    compile_no_operand,         // TOKEN_RET
    compile_no_operand,         // TOKEN_RLC
    compile_no_operand,         // TOKEN_RM
    compile_no_operand,         // TOKEN_RNC
    compile_no_operand,         // TOKEN_RNZ
    compile_no_operand,         // TOKEN_RP
    compile_no_operand,         // TOKEN_RPE
    compile_no_operand,         // TOKEN_RPO
    compile_no_operand,         // TOKEN_RRC
    compile_no_operand,         // TOKEN_RZ
   
    compile_reg_or_mem,         // TOKEN_SBB
    compile_hex8_operand,       // TOKEN_SBI
    compile_hex16_operand,      // TOKEN_SHLD
    compile_no_operand,         // TOKEN_SPHL
    compile_hex16_operand,      // TOKEN_STA
    compile_ldax,               // TOKEN_STAX
    compile_no_operand,         // TOKEN_STC
    compile_reg_or_mem,         // TOKEN_SUB
    compile_hex8_operand,       // TOKEN_SUI

    compile_no_operand,         // TOKEN_XCHG
    compile_reg_or_mem,         // TOKEN_XRA
    compile_hex8_operand,       // TOKEN_XRI
    compile_no_operand,         // TOKEN_XTHL

    // Special purpose tokens.
    compile_unexpected_token,   // TOKEN_ERROR
    compile_unexpected_token    // TOKEN_EOF
};

// Patch all the pending labels if they
// are declared yet, otherwise report
// errors to the user.
CompilationStatus patch_labels(){
    CompilationStatus ret = COMPILE_OK;
    u16 bak = *offset;
    for(u16 i = 0;i < pendingPointer;i++){
        if(labelTable[pending_labels[i].idx].isDeclared == 1){
            *offset = pending_labels[i].offset;
            write_dword(labelTable[pending_labels[i].idx].offset);
        }
        else if(pending_labels[i].decl_shown == 0){
            pwarn("Label used but not declared yet!");
            token_highlight_source(pending_labels[i].token);
            ret = LABELS_PENDING;
            pending_labels[i].decl_shown = 1;
        }
    }
    *offset = bak;
    return ret;
}

// Check for and report the presence
// of pending labels.
// This is usually called by the inline
// assembler 'asm', after the user has
// invoked 'exit'.
void compiler_report_pending(){
    u16 count = 0;
    for(u16 i = 0;i < pendingPointer;i++){
        if(labelTable[pending_labels[i].idx].isDeclared == 0){
            pwarn("Label '%.*s' is not declared!", pending_labels[i].token.length,
                    pending_labels[i].token.start);
            count++;
        }
    }
    if(count > 0){
        pwarn("%" Pu16 " label%s %s used but not declared!", count, count > 1 ? "s" : "", count > 1 ? "are" : "is");
        pinfo("To avoid erroneous results, patch %s manually before execution.", count > 1 ? "them" : "it");
    }
}

// Reset the internal states of the compiler
void compiler_reset(){
    for(siz i = 0;i < labelPointer;i++)
        free(labelTable[i].label);
    labelPointer = 0;
    for(siz i = 0;i < pendingPointer;i++){
        free((char *)pending_labels[i].token.start);
        pending_labels[i].decl_shown = 0;
    }
    pendingPointer = 0;

    memory = NULL;
    offset = NULL;
    memSize = 0;
    memory_full = 0;

    presentToken = (Token){TOKEN_ERROR, NULL, 0, 0, 0};
    previousToken = (Token){TOKEN_ERROR, NULL, 0, 0, 0};
}

// The driver
CompilationStatus compile(const char *source, u8 *mem, u16 size, u16 *off){
    initScanner(source);

    presentToken = (Token){TOKEN_ERROR, NULL, 0, 0, 0};
    previousToken = (Token){TOKEN_ERROR, NULL, 0, 0, 0};
    
    memory = mem;
    memSize = size;
    offset = off;

    Token t;
    CompilationStatus lastStatus = COMPILE_OK;
    while(lastStatus == COMPILE_OK && !memory_full && (t = scanToken()).type != TOKEN_EOF)
        lastStatus = compilationTable[t.type](t);

    if(memory_full)
        lastStatus = MEMORY_FULL;

    if(lastStatus == COMPILE_OK)
        return patch_labels();
    
    return lastStatus;
}
