#include <stdio.h>

#include "bytecode.h"
#include "display.h"

static const char* bytecode_strings[] = {
    #define INSTRUCTION(name, length)   #name,
    #include "instruction.h"
    #undef INSTRUCTION

    "lxi   sp",
    "dcx   sp",
    "inx   sp",
    "mov_r   ",
    "mvi   m",
    "mov   m",
    "dcr   m",
    "inr   m",
    "sub   m",
    "add   m",
    "ana   m",
    "ora   m",
    "xra   m",
    "cmp   m",
    "adc   m",
    "dad   sp"
};

const char* bytecode_get_string(Bytecode code){
    return bytecode_strings[code];
}

typedef void (*disassembleFn)(u8 *memory, u16 *pointer);

static void print_reg(u8 *memory, u16 *pointer){
    char reg = 'X';
    switch(memory[*pointer]){
        case REG_A:
            reg = 'a';
            break;
        case REG_B:
            reg = 'b';
            break;
        case REG_C:
            reg = 'c';
            break;
        case REG_D:
            reg = 'd';
            break;
        case REG_E:
            reg = 'e';
            break;
        case REG_H:
            reg = 'h';
            break;
        case REG_L:
            reg =  'l';
            break;
    }
    pgrn(" %c", reg);
    (*pointer)++;
}

static void dis_reg(u8 *memory, u16 *pointer){
    print_reg(memory, pointer);
}

static void dis_no_operand(u8 *memory, u16 *pointer){
    (void)memory; (void)pointer;
}

static void dis_hex8_operand(u8 *memory, u16 *pointer){
    pylw(" %xh", memory[*pointer]);
    (*pointer)++;
}

static void dis_hex16_operand(u8 *memory, u16 *pointer){
    pylw(" %x%xh", memory[*pointer + 1], memory[*pointer]);
    (*pointer) += 2;
}

static void dis_mov(u8 *memory, u16 *pointer){
    print_reg(memory, pointer);
    printf(",");
    print_reg(memory, pointer);
}

static void dis_lxi(u8 *memory, u16 *pointer){
    print_reg(memory, pointer);
    printf(",");
    pylw(" %x%xh", memory[*pointer + 1], memory[*pointer]);
    (*pointer) += 2;
}

static void dis_mvi(u8 *memory, u16 *pointer){
    print_reg(memory, pointer);
    printf(",");
    pylw(" %xh", memory[*pointer]);
    (*pointer) += 1;
}

static disassembleFn disassembleTable[] = {

    // Keywords.
    dis_hex8_operand,       // TOKEN_ACI
    dis_reg,                // TOKEN_ADC
    dis_reg,         // TOKEN_ADD
    dis_hex8_operand,       // TOKEN_ADDI
    dis_reg,         // TOKEN_ANA
    dis_hex8_operand,       // TOKEN_ANI
    
    dis_hex16_operand,      // TOKEN_CALL
    dis_hex16_operand,      // TOKEN_CC
    dis_hex16_operand,      // TOKEN_CM
    dis_no_operand,         // TOKEN_CMA
    dis_no_operand,         // TOKEN_CMC
    dis_reg,         // TOKEN_CMP
    dis_hex16_operand,      // TOKEN_CNC
    dis_hex16_operand,      // TOKEN_CNZ
    dis_hex16_operand,      // TOKEN_CP
    dis_hex8_operand,       // TOKEN_CPI
    dis_hex16_operand,      // TOKEN_CZ

    dis_no_operand,         // TOKEN_DAA
    dis_reg,                // TOKEN_DAD
    dis_reg,         // TOKEN_DCR
    dis_reg,      // TOKEN_DCX

    dis_no_operand,         // TOKEN_HLT
    
    dis_no_operand,         // TOKEN_IN
    dis_reg,         // TOKEN_INR
    dis_reg,      // TOKEN_INX
 
    dis_hex16_operand,      // TOKEN_JC
    dis_hex16_operand,      // TOKEN_JM
    dis_hex16_operand,      // TOKEN_JMP
    dis_hex16_operand,      // TOKEN_JNC
    dis_hex16_operand,      // TOKEN_JNZ
    dis_hex16_operand,      // TOKEN_JP
    dis_hex16_operand,      // TOKEN_JZ   
    
    dis_hex16_operand,      // TOKEN_LDA
    dis_reg,                // TOKEN_LDAX
    dis_hex16_operand,      // TOKEN_LHLD
    dis_lxi,                // TOKEN_LXI
    
    dis_mov,                // TOKEN_MOV
    dis_mvi,                // TOKEN_MVI
    
    dis_no_operand,         // TOKEN_NOP 
 
    dis_reg,         // TOKEN_ORA
    dis_hex8_operand,       // TOKEN_ORI
    dis_no_operand,         // TOKEN_OUT   
    
    dis_no_operand,         // TOKEN_RAL
    dis_no_operand,         // TOKEN_RAR
    dis_no_operand,         // TOKEN_RC
    dis_no_operand,         // TOKEN_RET
    dis_no_operand,         // TOKEN_RLC
    dis_no_operand,         // TOKEN_RM
    dis_no_operand,         // TOKEN_RNC
    dis_no_operand,         // TOKEN_RNZ
    dis_no_operand,         // TOKEN_RP
    dis_no_operand,         // TOKEN_RRC
    dis_no_operand,         // TOKEN_RZ
    
    dis_hex16_operand,      // TOKEN_STA
    dis_reg,            // TOKEN_STAX
    dis_reg,         // TOKEN_SUB
    dis_hex8_operand,       // TOKEN_SUI

    dis_reg,         // TOKEN_XRA
    dis_hex8_operand,       // TOKEN_XRI
    
    dis_hex16_operand,      // lxi_SP
    dis_no_operand,         // dcx sp
    dis_no_operand,         // inx sp
    dis_reg,              // mov_r
    dis_hex8_operand,       // mvi_m
    dis_reg,                // mov_m
    dis_no_operand,         // dcr m
    dis_no_operand,         // inr m 
    dis_no_operand,         // sub m
    dis_no_operand,         // add m
    dis_no_operand,         // ana m
    dis_no_operand,         // xra m
    dis_no_operand,         // ora m
    dis_no_operand,         // cmp m
    dis_no_operand,         // adc m
    dis_no_operand,         // dad sp
};

void bytecode_disassemble(u8 *memory, u16 pointer){
    pred("\n%04x:\t", pointer);
    pblue("%-5s", bytecode_strings[memory[pointer]]);
    pointer++;
    disassembleTable[memory[pointer - 1]](memory, &pointer);
}

void bytecode_disassemble_in_context(u8 *memory, u16 pointer, Machine *m){
    pred("\n%04x:\t", pointer);
    pblue("%-5s", bytecode_strings[memory[pointer]]);
    pointer++;
    Bytecode code = (Bytecode)memory[pointer - 1];
    disassembleTable[memory[pointer - 1]](memory, &pointer);
    
    #define FROM_PAIR(x, y)     (((u16)m->registers[x] << 8) | m->registers[y])
    #define FROM_HL()           FROM_PAIR(5, 6)
    #define GET_PAIR(num)       (num == REG_B ? "BC" : num == REG_D ? "DE" : "HL")

    switch(code){
        case BYTECODE_mov_R:
        case BYTECODE_mov_M:
        case BYTECODE_mvi_M:
        case BYTECODE_add_M:
        case BYTECODE_inr_M:
        case BYTECODE_sub_M:
        case BYTECODE_dcr_M:
        case BYTECODE_ana_M:
        case BYTECODE_ora_M:
        case BYTECODE_xra_M:
        case BYTECODE_cmp_M:
        case BYTECODE_adc_M:
            printf("\t(HL : 0x%x, memory[0x%x] : 0x%x)", FROM_HL(), FROM_HL(), memory[FROM_HL()]);
            break;
        case BYTECODE_ldax:
        case BYTECODE_stax:
            {
                u16 addr = FROM_PAIR(memory[pointer - 1], memory[pointer - 1] + 1);
                printf("\t(%s : 0x%x, memory[0x%x] : 0x%x)", GET_PAIR(memory[pointer - 1]), 
                        addr, addr, memory[addr]);
                break;
            }
        default:
            break;
    }
}

void bytecode_disassemble_chunk(u8 *memory, u16 pointer, u16 upto){
    while(pointer < upto){
        pred("\n%04x:\t", pointer);
        pblue("%-5s", bytecode_strings[memory[pointer]]);
        pointer++;
        disassembleTable[memory[pointer - 1]](memory, &pointer);
    }
}
