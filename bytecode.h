#pragma once

typedef enum{
    /* Data transfer instructions */
    MVI,    // mvi b, 47h
    MVI_M,  // mvi m, 54h
    MOV,    // mov b, a (between two registers)
    MOV_M,  // mov m, c (copy to memory)
    MOV_R,  // mov b, m (copy to register)
    LXI,    // lxi b, 2050h
    LXI_SP, // lxi sp, 3277h
    LDA,    // lda 2050h
    STA,    // sta 2070h
    LDAX,   // ldax b
    STAX,   // stax b
    OUT,    // out 01h
    IN,     // in 07h

    /* Arithmetic instructions */
    ADD,    // add b 
    ADD_M,  // add m
    ADI,    // addi 37h
    SUB,    // sub c
    SUB_M,  // sub m
    SUI,    // sui 7fh
    INR,    // inr d
    INR_M,  // inr m
    DCR,    // dcr c
    DCR_M,  // dcr m
    INX,    // inx h
    INX_SP, // inx sp
    DCX,    // dcx b
    DCX_SP, // dcx sp

    /* Logic and bit manipulation instructions */
    ANA,    // ana b
    ANA_M,  // ana m
    ANI,    // ani 2fh
    ORA,    // ora e
    ORA_M,  // ora m
    ORI,    // ori 3fh
    XRA,    // xra c
    XRA_M,  // xra m
    XRI,    // xri 6ah
    CMP,    // cmp b
    CMP_M,  // cmp m
    CPI,    // cpi 4fh
    CMA,    // cma
    RLC,    // rlc
    RAL,    // ral
    RRC,    // rrc
    RAR,    // rar

    /* Braching instructions */
    JMP,    // jmp 2050h
    JZ,     // jz 2080h
    JNZ,    // jnz 2070h
    JC,     // jc 2025h
    JNC,    // jnc 2030h
    JP,     // jp 2323h
    JM,     // jm 3245h
    CALL,   // call 2075h
    CC,     // cc 3423h
    CNC,    // cnc 4433h
    CZ,     // cz 4343h
    CNZ,    // cnz 1923h
    CP,     // cp 3211h
    CM,     // cm 1292h
    RET,    // ret
    RC,     // rc
    RNC,    // rnc
    RZ,     // rz
    RNZ,    // rnz
    RM,     // rm
    RP,     // rp

    /* Machine control instructions */
    HLT,
    NOP
} Bytecode;
