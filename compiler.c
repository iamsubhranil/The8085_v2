#include <memory.h>

#include "display.h"
#include "compiler.h"
#include "scanner.h"
#include "bytecode.h"
#include "vm.h"

// We statically allocate labels to reduce overhead
#define NUM_LABELS  32

typedef struct{
    const char *label;  // name of the label
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
} PendingLabel;

// C style list of labels
static Label labelTable[NUM_LABELS] = {{NULL, 0, 0, 0}};
static siz labelPointer = 0;

// C style list of pending labels
static PendingLabel pending_labels[NUM_PENDING_LABELS] = {{{}, 0, 0}};
static siz pendingPointer = 0;

// Memory management for actually writing bytes
static u16 memSize = 0, *offset = NULL;
static u8 *memory = NULL;

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
u16 write_byte(u8 value){
    memory[*offset] = value;
    (*offset)++;
    return (*offset) - 1;
}

// Write two bytes to the memory
u16 write_dword(u16 value){
    write_byte(value & 0x00ff);
    write_byte((value & 0xff00) >> 8);
    return (*offset) - 2;
}

// The typical message to be shown when an operand
// is of an unexpected type
static void unexpected_operand(const char *expected, Bytecode code, Token t){
    perr("Expected %s after " ANSI_FONT_BOLD "%s" ANSI_COLOR_RESET "!",
            expected, bytecode_get_string(code));
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

// Here's a bit of explanation of the approach
// and the architechture of the vm and how it maps
// to the actual 8085 ISA.
//
// The thing is, 8085 had a lot of ambiguous
// opcodes. For example, one 'add' instruction
// can add the accumulator with the content
// at a memory address or with just another
// register. 8085 usually tackled it by denoting
// the memory as either 110 or 111(I can't remember
// correctly now), in the opcode. Which then, the
// decoder would process and do the required thing.
// Since we're not following the internal's of the
// architechture pie to pie, just the ISA part, and
// since we don't have such a compilcated decoder
// (which will just make these type of instructions
// a hell lot complicated with a bunch of `if`s
// hanging out with each of them), we're
// distinguishing the register and the memory
// version at the opcode level, and emitting the
// specific opcode while compilation itself,
// to benefit the runtime.
// So that, we have two 'add' instructions at the vm
// level : 
// 1. One that adds two registers, i.e. the usual 
// 'add', 
// 2. One that adds the accumulator with a memory 
// address - 'add_M'.
// Now, we obviously don't want to confuse the
// programmers with the internals of the specific
// architechture of our vm, we just want them
// to remember how 8085 does things. So, while
// compilation, whenever we're seeing a statement
// like 'add m' or 'dad sp', we're replacing the
// instructions with their specific counterparts,
// to make our little vm breathe a little.
// You can see instruction.h, which maps each
// instructions exactly with 8085, and then
// when you see bytecodes.h, which is our version
// of that instruction set, you will see after
// all the instructions from instruction.h,
// some more opcodes are declared 
// with suffix _M, or _SP, or _PSW, which
// corresponds to the specific 'add m', or
// 'dad sp' or 'push psw' instructions.
// The compiler just maps each 8085 opcode to
// the specific opcode required for our vm to
// run.

// Get the _M version of opcode
// add m
static Bytecode get_m_version_of(Bytecode code){
    switch(code){
        case BYTECODE_mvi:
            return BYTECODE_mvi_M;
        case BYTECODE_mov:
            return BYTECODE_mov_M;
        case BYTECODE_dcr:
            return BYTECODE_dcr_M;
        case BYTECODE_inr:
            return BYTECODE_inr_M;
        case BYTECODE_add:
            return BYTECODE_add_M;
        case BYTECODE_sub:
            return BYTECODE_sub_M;
        case BYTECODE_ana:
            return BYTECODE_ana_M;
        case BYTECODE_xra:
            return BYTECODE_xra_M;
        case BYTECODE_cmp:
            return BYTECODE_cmp_M;
        case BYTECODE_adc:
            return BYTECODE_adc_M;
        case BYTECODE_sbb:
            return BYTECODE_sbb_M;
        default:
            perr("[Internal Error] M version required for code %d", code);
            token_highlight_source(presentToken);
            return BYTECODE_hlt;
    }
}

// Get the _SP version of the opcode
// dad sp
static Bytecode get_sp_version_of(Bytecode code){
    switch(code){
        case BYTECODE_lxi:
            return BYTECODE_lxi_SP;
        case BYTECODE_inx:
            return BYTECODE_inx_SP;
        case BYTECODE_dcx:
            return BYTECODE_dcx_SP;
        case BYTECODE_dad:
            return BYTECODE_dad_SP;
        default:
            perr("[Internal error] SP version required for code %d", code);
            token_highlight_source(presentToken);
            return BYTECODE_hlt;
    }
}

// Get the _PSW version of the opcode
// push psw
static Bytecode get_psw_version_of(Bytecode code){
    switch(code){
        case BYTECODE_pop:
            return BYTECODE_pop_PSW;
        case BYTECODE_push:
            return BYTECODE_push_PSW;
        default:
            perr("[Internal Error] PSW version required for code %d", code);
            token_highlight_source(presentToken);
            return BYTECODE_hlt;
    }
}

// Each bytecode mapping to a
// specific type of token, since
// all tokens here are mostly
// instructions except for a few
static Bytecode bytecodes[] = {
    BYTECODE_hlt,    // colon
    BYTECODE_hlt,    // comma
    BYTECODE_hlt,    // identifer
    BYTECODE_hlt,    // number

    #define INSTRUCTION(name, length)   BYTECODE_##name,
    #include "instruction.h"
    #undef INSTRUCTION

    BYTECODE_hlt,    // error
    BYTECODE_hlt     // eof
};

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
    if(!consume(TOKEN_COLON, "Expected ':' after label!"))
        return PARSE_ERROR;
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
    labelTable[labelPointer].label = t.start;
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
                labelTable[labelPointer].label = t.start;
                labelTable[labelPointer].length = t.length;
                labelTable[labelPointer].offset = 0;
                labelTable[labelPointer].isDeclared = 0;
                labelPointer++;
            }

            pending_labels[pendingPointer].idx = found ? idx : (labelPointer - 1);
            pending_labels[pendingPointer].offset = *offset;
            pending_labels[pendingPointer].token = t;
            pendingPointer++;
            (*offset) += 2;
            return COMPILE_OK;
        }
        perr("Expected %d bit number!", (8*(is16 + 1)));
        token_highlight_source(t);
        return PARSE_ERROR;
    }
    if(!consume(TOKEN_IDENTIFIER, "Expected 'h' after number!")){
        return PARSE_ERROR;
    }
    if(presentToken.length != 1 || presentToken.start[0] != 'h'){
        perr("Expected 'h' after number!");
        token_highlight_source(presentToken);
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
        write_byte(number & 0x00ff);
    return COMPILE_OK;
}

// Compile a register.
// The token must be validated to
// be a register beforehand, which is
// usually done by the parent methods
// which invoke this.
static CompilationStatus compile_reg(Token t){
    switch(t.start[0]){
        case 'a': write_byte(REG_A); break;
        case 'b': write_byte(REG_B); break;
        case 'c': write_byte(REG_C); break;
        case 'd': write_byte(REG_D); break;
        case 'e': write_byte(REG_E); break;
        case 'h': write_byte(REG_H); break;
        case 'l': write_byte(REG_L); break;
    }
    return COMPILE_OK;
}

// Compile an instruction with no operand
static CompilationStatus compile_no_operand(Token t){
    write_byte(bytecodes[t.type]);
    return COMPILE_OK;
}

// Compile an instruction with one 8 bit
// immediate operand
static CompilationStatus compile_hex8_operand(Token t){
    write_byte(bytecodes[t.type]);
    return compile_hex(0);
}

// Compile an instruction with one 16 bit
// immediate operand
static CompilationStatus compile_hex16_operand(Token t){
    write_byte(bytecodes[t.type]);
    return compile_hex(1);
}

// Compile an instruction of type
// opcode [r/m]
static CompilationStatus compile_reg_or_mem(Token t){
    Bytecode code = bytecodes[t.type];

    if(isreg(advance())){ // check whether or not the next token is a register
        write_byte(code);
        compile_reg(presentToken);
    }
    else if(ismem(presentToken)){
        write_byte(get_m_version_of(code)); // get the m version
    }
    else{
        unexpected_operand("register or memory", code, presentToken);
        return PARSE_ERROR;
    }
    return COMPILE_OK;
}

// Compile an instruction of type
// opcode regpair
static CompilationStatus compile_regpair(Token t){
    Bytecode code = bytecodes[t.type];
    if(isregpair(advance())){
        write_byte(code);
        compile_reg(presentToken);
        return COMPILE_OK;
    }
    else{
        unexpected_operand("register pair", code, presentToken);
        return PARSE_ERROR;
    }
}

// Compile an instruction of type
// opcode [regpair/sp]
static CompilationStatus compile_regpair_or_sp(Token t){
    Bytecode code = bytecodes[t.type];

    if(isregpair(advance())){ // check whether or not the next token is a register pair
        write_byte(code);
        compile_reg(presentToken);
    }
    else if(issp(presentToken)){
        write_byte(get_sp_version_of(code)); // get the sp version
    }
    else{
        unexpected_operand("register pair or stack pointer", code, presentToken);
        return PARSE_ERROR;
    }
    return COMPILE_OK;
}

// Compile an instruction of type
// opcode [regpair/psw]
static CompilationStatus compile_regpair_or_psw(Token t){
    Bytecode code = bytecodes[t.type];

    if(isregpair(advance())){ // check whether or not the next token is a register pair
        write_byte(code);
        compile_reg(presentToken);
    }
    else if(ispsw(presentToken)){
        write_byte(get_psw_version_of(code)); // get the psw version
    }
    else{
        unexpected_operand("register pair or program status word", code, presentToken);
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
    Bytecode code = bytecodes[t.type];

    if(isreg(advance())){
        Token prevreg = presentToken;
        if(!consume(TOKEN_COMMA, "Expected comma between operands!"))
            return PARSE_ERROR;
        if(isreg(advance())){
            write_byte(code);
            compile_reg(prevreg);
            compile_reg(presentToken);
            return COMPILE_OK;
        }
        else if(ismem(presentToken)){
            write_byte(BYTECODE_mov_R);
            compile_reg(prevreg);
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
            write_byte(BYTECODE_mov_M);
            compile_reg(presentToken);
            return COMPILE_OK;
        }
        else{
            perr("Expected register!"); 
            token_highlight_source(presentToken);
            return PARSE_ERROR;
        }
    }
    else{
        unexpected_operand("register or memory", code, presentToken);
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
    compile_hex8_operand,       // TOKEN_CPI
    compile_hex16_operand,      // TOKEN_CZ

    compile_no_operand,         // TOKEN_DAA
    compile_regpair_or_sp,      // TOKEN_DAD
    compile_reg_or_mem,         // TOKEN_DCR
    compile_regpair_or_sp,      // TOKEN_DCX

    compile_no_operand,         // TOKEN_HLT
    
    compile_no_operand,         // TOKEN_IN
    compile_reg_or_mem,         // TOKEN_INR
    compile_regpair_or_sp,      // TOKEN_INX
 
    compile_hex16_operand,      // TOKEN_JC
    compile_hex16_operand,      // TOKEN_JM
    compile_hex16_operand,      // TOKEN_JMP
    compile_hex16_operand,      // TOKEN_JNC
    compile_hex16_operand,      // TOKEN_JNZ
    compile_hex16_operand,      // TOKEN_JP
    compile_hex16_operand,      // TOKEN_JZ   
    
    compile_hex16_operand,      // TOKEN_LDA
    compile_regpair,            // TOKEN_LDAX
    compile_hex16_operand,      // TOKEN_LHLD
    compile_lxi,                // TOKEN_LXI
    
    compile_mov,                // TOKEN_MOV
    compile_mvi,                // TOKEN_MVI
    
    compile_no_operand,         // TOKEN_NOP 
 
    compile_reg_or_mem,         // TOKEN_ORA
    compile_hex8_operand,       // TOKEN_ORI
    compile_no_operand,         // TOKEN_OUT   
   
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
    compile_no_operand,         // TOKEN_RRC
    compile_no_operand,         // TOKEN_RZ
   
    compile_reg_or_mem,         // TOKEN_SBB
    compile_hex8_operand,       // TOKEN_SBI
    compile_hex16_operand,      // TOKEN_SHLD
    compile_no_operand,         // TOKEN_SPHL
    compile_hex16_operand,      // TOKEN_STA
    compile_regpair,            // TOKEN_STAX
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
        else{
            pwarn("Label used but not declared yet!");
            token_highlight_source(pending_labels[i].token);
            ret = LABELS_PENDING;
        }
    }
    *offset = bak;
    return ret;
}

// The driver
CompilationStatus compile(const char *source, u8 *mem, u16 size, u16 *off){
    initScanner(source);
    
    memory = mem;
    memSize = size;
    offset = off;

    Token t;
    CompilationStatus lastStatus = COMPILE_OK;
    while((t = scanToken()).type != TOKEN_EOF && lastStatus == COMPILE_OK)
        lastStatus = compilationTable[t.type](t);

    if(lastStatus == COMPILE_OK)
        return patch_labels();
    
    return lastStatus;
}
