#pragma once

#include "common.h"
#include "vm.h"

typedef enum{
    #define INSTRUCTION(name, length)   BYTECODE_##name,
    #include "instruction.h"
    #undef INSTRUCTION

    BYTECODE_lxi_SP, // lxi sp, 3277h
    BYTECODE_dcx_SP, // dcx sp
    BYTECODE_inx_SP, // inx sp
    BYTECODE_mov_R,  // mov b, m (copy to register)
    BYTECODE_mvi_M,  // mvi m, 54h
    BYTECODE_mov_M,  // mov m, c (copy to memory)
    BYTECODE_dcr_M,  // dcr m
    BYTECODE_inr_M,  // inr m
    BYTECODE_sub_M,  // sub m
    BYTECODE_add_M,  // add m
    BYTECODE_ana_M,  // ana m
    BYTECODE_ora_M,  // ora m
    BYTECODE_xra_M,  // xra m
    BYTECODE_cmp_M,  // cmp m
    BYTECODE_adc_M,  // adc m
} Bytecode;

const char* bytecode_get_string(Bytecode code);
// disassemble only one instruction
void bytecode_disassemble(u8 *memory, u16 pointer);
// continue disassembly until pointer < upto
void bytecode_disassemble_chunk(u8 *memory, u16 pointer, u16 upto);  
// disassemble instruction with context to the present state of the machine
void bytecode_disassemble_in_context(u8 *memory, u16 pointer, Machine *m);
