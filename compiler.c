#include <memory.h>

#include "display.h"
#include "compiler.h"
#include "scanner.h"
#include "bytecode.h"
#include "vm.h"

// We statically allocate labels to reduce overhead
#define NUM_LABELS  32

typedef struct{
    Token t;
    const char *label;
    int length;
    u16 offset;
    u8 isDeclared;
} Label;

static Label labelTable[NUM_LABELS] = {{{},NULL, 0, 0, 0}};
static siz labelPointer = 0;

// Statically allocate pending labels too
#define NUM_PENDING_LABELS  32
typedef struct{
    u16 idx;
    u16 offset;
} PendingLabel;

static PendingLabel pending_labels[NUM_PENDING_LABELS] = {{0, 0}};
static siz pendingPointer = 0;

static u16 memSize = 0, *offset = NULL;
static u8 *memory = NULL;

u16 write_byte(u8 value){
    memory[*offset] = value;
    (*offset)++;
    return (*offset) - 1;
}

u16 write_dword(u16 value){
    write_byte(value & 0x00ff);
    write_byte((value & 0xff00) >> 8);
    return (*offset) - 2;
}

typedef CompilationStatus (*compilerFn)(Token t);

CompilationStatus unexpected_token(Token t){
    perr("Unexpected token at line %d!", t.line);
    token_highlight_source(t);
    return PARSE_ERROR;
}

static void unexpected_operand(const char *expected, Bytecode code, Token t){
    perr("Expected %s after " ANSI_FONT_BOLD "%s" ANSI_COLOR_RESET "!",
            expected, bytecode_get_string(code));
    token_highlight_source(t);
}

static Token presentToken = {}, previousToken = {};

static Token advance(){
    if(presentToken.type == TOKEN_EOF)
        return presentToken;
    previousToken = presentToken;
    presentToken = scanToken();
    return presentToken;
}

static bool consume(TokenType type, const char *message){
    if(advance().type != type){
        perr("%s", message);
        token_highlight_source(presentToken);
        return false;
    }
    return true;
}

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
                labelTable[labelPointer].t = t;
                labelPointer++;
            }

            pending_labels[pendingPointer].idx = found ? idx : (labelPointer - 1);
            pending_labels[pendingPointer].offset = *offset;
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
            token_highlight_source(labelTable[pending_labels[i].idx].t);
            ret = LABELS_PENDING;
        }
    }
    *offset = bak;
    return ret;
}

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

static CompilationStatus compile_hex16_operand(Token t){
    write_byte(bytecodes[t.type]);
    return compile_hex(1);
}

static CompilationStatus compile_hex8_operand(Token t){
    write_byte(bytecodes[t.type]);
    return compile_hex(0);
}

static CompilationStatus compile_no_operand(Token t){
    write_byte(bytecodes[t.type]);
    return COMPILE_OK;
}

static bool isreg(Token t){
    return t.length == 1 && 
        ((t.start[0] >= 'a' && t.start[0] <= 'e') || t.start[0] == 'h' || t.start[0] == 'l');
}

static bool ismem(Token t){
    return t.length == 1 && t.start[0] == 'm';
}

static CompilationStatus compile_reg(Token t){
    if(!isreg(t)){
        perr("Expected register!");
        token_highlight_source(t);
        return PARSE_ERROR;
    }
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

static CompilationStatus compile_mvi(Token t){
    CompilationStatus st = COMPILE_OK;
    if((st = compile_reg_or_mem(t)) != COMPILE_OK)
        return st;
    if(!consume(TOKEN_COMMA, "Expected comma between operands"))
        return PARSE_ERROR;
    return compile_hex(0);
}

static bool isregpair(Token t){
    return t.length == 1 && (t.start[0] == 'b' || t.start[0] == 'd' || t.start[0] == 'h');
}

static bool issp(Token t){
    return t.length == 2 && t.start[0] == 's' && t.start[1] == 'p';
}

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

static bool ispsw(Token t){
    return t.type == TOKEN_IDENTIFIER && t.length == 3
        && t.start[0] == 'p' && t.start[1] == 's' && t.start[2] == 'w';
}

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

static compilerFn compilationTable[] = {
    // Single-character tokens.
    unexpected_token,           // TOKEN_COLON
    unexpected_token,           // TOKEN_COMMA

    // Literals.
    compile_label,              // TOKEN_IDENTIFIER
    unexpected_token,           // TOKEN_NUMBER

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
    unexpected_token,           // TOKEN_ERROR
    unexpected_token            // TOKEN_EOF
};

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
