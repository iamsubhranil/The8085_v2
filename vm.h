#pragma once

#include "common.h"

#define REG_A   0
#define REG_B   1
#define REG_C   2
#define REG_D   3
#define REG_E   4
#define REG_H   5
#define REG_L   6
#define REG_FL  7

#define FLG_S   7
#define FLG_Z   6
#define FLG_A   4
#define FLG_P   2
#define FLG_C   0

typedef struct{
    // 0 -> A
    // 1 -> B
    // 2 -> C
    // 3 -> D
    // 4 -> E
    // 5 -> H
    // 6 -> L
    // 7 -> Flags -> Sign Zero -- AuxiliaryCarry -- Parity -- Carry
    u8 registers[8];
    // The program counter
    u16 pc;
    // The stack pointer
    u16 sp;
} Machine;

void print_machine(Machine *m);
void run(Machine *m, u8 *memory);
