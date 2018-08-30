#include "bytecode.h"

#ifdef USE_BCVM

#include <stdio.h>

#include "display.h"

// Table containing strings corresponding
// to each bytecode. 
// Extra strings for the unambiguous
// instructions of the vm.
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
    "dad   sp",
    "pop   psw",
    "push  psw",
    "sbb   m",
};

// Returns the string corresponding to
// a bytecode
const char* bytecode_get_string(Bytecode code){
    return bytecode_strings[code];
}

// Prints the register character
// corresponding to a register id
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
    if(reg == 'X')
        pred(ANSI_FONT_BOLD " X");
    else
        pgrn(" %c", reg);
    (*pointer)++;
}

// Function type which disassembles an instruction
// at present pointer in memory
typedef void (*disassembleFn)(u8 *memory, u16 *pointer);

// Disassemble an instruction containing no
// operands
static void dis_no_operand(u8 *memory, u16 *pointer){
    (void)memory; (void)pointer;
}

// Disassemble an instruction containing one
// register as the operand
static void dis_reg(u8 *memory, u16 *pointer){
    print_reg(memory, pointer);
}

// Disassemble an instruction containing one
// immediate 8 bit operand
static void dis_hex8_operand(u8 *memory, u16 *pointer){
    pylw(" %xh", memory[*pointer]);
    (*pointer)++;
}

// Disassemble an instruction containing one
// immediate 16 bit operand
static void dis_hex16_operand(u8 *memory, u16 *pointer){
    pylw(" %x%xh", memory[*pointer + 1], memory[*pointer]);
    (*pointer) += 2;
}

// Special disassembly functions for some
// instructions

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

// Table which correspond one vm opcode to
// a disassembly function.
// Note that we're not talking about
// 8085 opcode, we're talking about the
// opcodes of our vm, so we also have to
// include those "special" unambiguous
// opcodes.
static disassembleFn disassembleTable[] = {

    // Keywords.
    dis_hex8_operand,       // BYTECODE_ACI
    dis_reg,                // BYTECODE_ADC
    dis_reg,                // BYTECODE_ADD
    dis_hex8_operand,       // BYTECODE_ADDI
    dis_reg,                // BYTECODE_ANA
    dis_hex8_operand,       // BYTECODE_ANI
    
    dis_hex16_operand,      // BYTECODE_CALL
    dis_hex16_operand,      // BYTECODE_CC
    dis_hex16_operand,      // BYTECODE_CM
    dis_no_operand,         // BYTECODE_CMA
    dis_no_operand,         // BYTECODE_CMC
    dis_reg,                // BYTECODE_CMP
    dis_hex16_operand,      // BYTECODE_CNC
    dis_hex16_operand,      // BYTECODE_CNZ
    dis_hex16_operand,      // BYTECODE_CP
    dis_hex16_operand,      // BYTECODE_CPE
    dis_hex8_operand,       // BYTECODE_CPI
    dis_hex16_operand,      // BYTECODE_CPO
    dis_hex16_operand,      // BYTECODE_CZ

    dis_no_operand,         // BYTECODE_DAA
    dis_reg,                // BYTECODE_DAD
    dis_reg,                // BYTECODE_DCR
    dis_reg,                // BYTECODE_DCX

    dis_no_operand,         // BYTECODE_HLT
    
    dis_hex8_operand,       // BYTECODE_IN
    dis_reg,                // BYTECODE_INR
    dis_reg,                // BYTECODE_INX
 
    dis_hex16_operand,      // BYTECODE_JC
    dis_hex16_operand,      // BYTECODE_JM
    dis_hex16_operand,      // BYTECODE_JMP
    dis_hex16_operand,      // BYTECODE_JNC
    dis_hex16_operand,      // BYTECODE_JNZ
    dis_hex16_operand,      // BYTECODE_JP
    dis_hex16_operand,      // BYTECODE_JPE
    dis_hex16_operand,      // BYTECODE_JPO
    dis_hex16_operand,      // BYTECODE_JZ   
    
    dis_hex16_operand,      // BYTECODE_LDA
    dis_reg,                // BYTECODE_LDAX
    dis_hex16_operand,      // BYTECODE_LHLD
    dis_lxi,                // BYTECODE_LXI
    
    dis_mov,                // BYTECODE_MOV
    dis_mvi,                // BYTECODE_MVI
    
    dis_no_operand,         // BYTECODE_NOP 
 
    dis_reg,                // BYTECODE_ORA
    dis_hex8_operand,       // BYTECODE_ORI
    dis_hex8_operand,       // BYTECODE_OUT   
   
    dis_no_operand,         // BYTECODE_PCHL
    dis_reg,                // BYTECODE_POP
    dis_reg,                // BYTECODE_PUSH

    dis_no_operand,         // BYTECODE_RAL
    dis_no_operand,         // BYTECODE_RAR
    dis_no_operand,         // BYTECODE_RC
    dis_no_operand,         // BYTECODE_RET
    dis_no_operand,         // BYTECODE_RLC
    dis_no_operand,         // BYTECODE_RM
    dis_no_operand,         // BYTECODE_RNC
    dis_no_operand,         // BYTECODE_RNZ
    dis_no_operand,         // BYTECODE_RP
    dis_no_operand,         // BYTECODE_RPE
    dis_no_operand,         // BYTECODE_RPO
    dis_no_operand,         // BYTECODE_RRC
    dis_no_operand,         // BYTECODE_RZ
   
    dis_reg,                // BYTECODE_SBB
    dis_hex8_operand,       // BYTECODE_SBI
    dis_hex16_operand,      // BYTECODE_SHLD
    dis_no_operand,         // BYTECODE_SPHL
    dis_hex16_operand,      // BYTECODE_STA
    dis_reg,                // BYTECODE_STAX
    dis_no_operand,         // BYTECODE_STC
    dis_reg,                // BYTECODE_SUB
    dis_hex8_operand,       // BYTECODE_SUI

    dis_no_operand,         // BYTECODE_XCHG
    dis_reg,                // BYTECODE_XRA
    dis_hex8_operand,       // BYTECODE_XRI
    dis_no_operand,         // BYTECODE_XTHL
    
    dis_hex16_operand,      // BYTECODE_lxi_SP
    dis_no_operand,         // BYTECODE_dcx_SP
    dis_no_operand,         // BYTECODE_inx_SP
    dis_reg,                // BYTECODE_mov_R
    dis_hex8_operand,       // BYTECODE_mvi_M
    dis_reg,                // BYTECODE_mov_M
    dis_no_operand,         // BYTECODE_dcr_M
    dis_no_operand,         // BYTECODE_inr_M 
    dis_no_operand,         // BYTECODE_sub_M
    dis_no_operand,         // BYTECODE_add_M
    dis_no_operand,         // BYTECODE_ana_M
    dis_no_operand,         // BYTECODE_xra_M
    dis_no_operand,         // BYTECODE_ora_M
    dis_no_operand,         // BYTECODE_cmp_M
    dis_no_operand,         // BYTECODE_adc_M
    dis_no_operand,         // BYTECODE_dad_SP
    dis_no_operand,         // BYTECODE_pop_PSW
    dis_no_operand,         // BYTECODE_push_PSW
    dis_no_operand,         // BYTECODE_sbb_M
};

static siz dis_table_siz = sizeof(disassembleTable)/sizeof(disassembleFn);

// Disassemble one instruction at the given offset
void bytecode_disassemble(u8 *memory, u16 pointer){
    pred("\n%04x:\t", pointer);
    pblue("%-5s", bytecode_strings[memory[pointer]]);
    pointer++;
    disassembleTable[memory[pointer - 1]](memory, &pointer);
}

// Disassemble one instruction at the given offset
// in present execution context. This basically
// shows some extra infos, like contents of the
// memory location that HL points to, as
// applicable to an instruction.
void bytecode_disassemble_in_context(u8 *memory, u16 pointer, Machine *m){
    pred("\n%04x:\t", pointer);
    if(memory[pointer] > dis_table_siz){
        pred( ANSI_FONT_BOLD "<invalid opcode>");
        return;
    }
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
        case BYTECODE_sbb_M:
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
        case BYTECODE_pop_PSW:
        case BYTECODE_push_PSW:
            {
                printf("\t(A : 0x%x, FLAGS : 0x%x)", m->registers[REG_A], m->registers[REG_FL]);
                break;
            }
        default:
            break;
    }
}

// Disassemble a whole chunk of bytecode until the
// specified address 'upto'
void bytecode_disassemble_chunk(u8 *memory, u16 pointer, u16 upto){
    while(pointer <= upto){
        pred("\n%04x:\t", pointer);
        if(memory[pointer] > dis_table_siz){
            pred( ANSI_FONT_BOLD "<invalid opcode>");
            pointer++;
            continue;
        }
        pblue("%-5s", bytecode_strings[memory[pointer]]);
        pointer++;
        disassembleTable[memory[pointer - 1]](memory, &pointer);
    }
}

#endif
