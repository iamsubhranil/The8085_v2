#pragma once

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
    BYTECODE_cmp_M  // cmp m
} Bytecode;
