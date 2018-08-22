#include "vm.h"

#ifdef USE_NEOVM

#include "common.h"
#include "display.h"
#include <stdio.h>
#include <time.h>

#define GET_FLAG(x)             ((m->registers[REG_FL] >> x) & 1)

#define NEXT_BYTE()         memory[m->pc++]
#define NEXT_DWORD()        ((u16)NEXT_BYTE() | ((u16)NEXT_BYTE() << 8))

#define FROM_PAIR(x, y)     (((u16)m->registers[x] << 8) | m->registers[y])
#define FROM_HL()           FROM_PAIR(REG_H, REG_L)

#define SET_FLAG(x)         (m->registers[REG_FL] |= (1 << x))
#define RESET_FLAG(x)       (m->registers[REG_FL] &= ~(1 << x))
#define CHANGE_FLAG(f, b)   (m->registers[REG_FL] ^= (-(unsigned long)(b) ^ m->registers[REG_FL]) & (1UL << f))

#define INIT_FLG_C(res)                                             \
    CHANGE_FLAG(FLG_C, (res > 0xff));

#define INIT_FLG_Z(res)                                             \
    CHANGE_FLAG(FLG_Z, ((res & 0xff) == 0));

#define INIT_FLG_S(res)                                             \
    CHANGE_FLAG(FLG_S, ((res >> 7) & 1));

#define INIT_FLG_A(x, y)                                            \
    CHANGE_FLAG(FLG_A, (((x) & 0x00ff) + ((y) & 0x00ff) > 0x00ff))

#define INIT_FLG_P(res)                                                 \
    u8 temp = res & 0xff;                                               \
    temp = temp - (temp >> 1) - (temp >> 2) - (temp >> 3)               \
    - (temp >> 4) - (temp >> 5) - (temp >> 6) - (temp >> 7);            \
    CHANGE_FLAG(FLG_P, !(temp & 1));

#define INIT_FLAGS(res)     \
    INIT_FLG_C(res)         \
    INIT_FLG_Z(res)         \
    INIT_FLG_S(res)         \
    INIT_FLG_P(res)

#define ADD()                                           \
    u16 res = with + m->registers[REG_A];               \
    INIT_FLAGS(res);                                    \
    INIT_FLG_A(with, m->registers[REG_A]);              \
    m->registers[REG_A] = res & 0xff;

#define ADD2()                                          \
    u16 res = with1 + with2 + m->registers[REG_A];      \
    INIT_FLAGS(res);                                    \
    INIT_FLG_A(with1 + with2, m->registers[REG_A]);     \
    m->registers[REG_A] = res & 0xff;

#define SUB()                           \
    u8 with = ~by + 1;                  \
    ADD();                              \
    CHANGE_FLAG(FLG_C, !GET_FLAG(FLG_C))

#define LOGICAL(op)                                     \
    m->registers[REG_A] = m->registers[REG_A] op with;

#define LOGICAL_NOT_CMA(op)         \
    LOGICAL(op)                     \
    RESET_FLAG(FLG_C);              \
    RESET_FLAG(FLG_A);              \
    INIT_FLG_S(m->registers[REG_A]) \
    INIT_FLG_Z(m->registers[REG_A]) \
    INIT_FLG_P(m->registers[REG_A]) \

#define JMP_ON(cond)            \
    u16 addr = NEXT_DWORD();    \
    tstates = 7;                \
    if(cond){                   \
        m->pc = addr;           \
        tstates = 10;           \
    }                           \
    break;

#define CALL_ON(cond)                               \
    u16 addr = NEXT_DWORD();                        \
    tstates = 9;                                    \
    if(cond){                                       \
        memory[m->sp - 1] = (m->pc & 0xff00) >> 8;  \
        memory[m->sp - 2] = m->pc & 0x00ff;         \
        m->sp -= 2;                                 \
        m->pc = addr;                               \
        tstates = 18;                               \
    }                                               \
    break;

#define RET_ON(cond)                        \
    tstates = 6;                            \
    if(cond){                               \
        m->pc = memory[m->sp];              \
        m->pc |= (memory[m->sp + 1] << 8);  \
        m->sp += 2;                         \
        tstates = 12;                       \
    }                                       \
    break;

#define DAD()                                   \
    u32 res = FROM_PAIR(REG_H, REG_L) + with;   \
    if(res > 0xffff)                            \
    SET_FLAG(FLG_C);                            \
    m->registers[REG_H] = (res & 0xff00) >> 8;  \
    m->registers[REG_L] = res & 0x00ff;         \
    tstates = 10;

#define ADC(reg)                                            \
    u8 with1 = m->registers[reg], with2 = GET_FLAG(FLG_C);  \
    ADD2();                                                 \
    tstates = 4;

#define ADD_R(reg)                  \
    u8 with = m->registers[reg];    \
    ADD();                          \
    tstates = 4;

#define ANA(reg)                \
    u8 with = m->registers[reg];\
    LOGICAL_NOT_CMA(&);         \
    SET_FLAG(FLG_A);            \
    tstates = 4;

#define CMP(reg)                    \
    u8 bak = m->registers[REG_A];   \
    u8 by = m->registers[reg] + 1;  \
    SUB();                          \
    m->registers[REG_A] = bak;      \
    tstates = 4;

#define DAD_R(reg)                      \
    u16 with = FROM_PAIR(reg, reg+1);   \
    DAD();

#define DCR(reg)                        \
    u16 res = m->registers[reg] - 1;    \
    INIT_FLG_S(res);                    \
    INIT_FLG_Z(res);                    \
    INIT_FLG_P(res);                    \
    INIT_FLG_A(m->registers[reg], -1);  \
    m->registers[reg] = res & 0xff;     \
    tstates = 4;

#define DCX(first)                              \
    u16 res = FROM_PAIR(first, first + 1) - 1;  \
    m->registers[first] = (res & 0xff00) >> 8;  \
    m->registers[first + 1] = res & 0x00ff;     \
    tstates = 6;                                \
    break;

#define INR(reg)                        \
    u16 res = m->registers[reg] + 1;    \
    INIT_FLG_S(res);                    \
    INIT_FLG_Z(res);                    \
    INIT_FLG_P(res);                    \
    INIT_FLG_A(m->registers[reg], 1);   \
    m->registers[reg] = res & 0xff;     \
    tstates = 4;

#define INX(first)                              \
    u16 res = FROM_PAIR(first, first + 1) + 1;  \
    m->registers[first] = (res & 0xff00) >> 8;  \
    m->registers[first + 1] = res & 0x00ff;     \
    tstates = 6;

#define LDAX(first)                         \
    u16 from = FROM_PAIR(first, first + 1); \
    m->registers[REG_A] = memory[from];     \
    tstates = 7;

#define LXI(first)                          \
    m->registers[first + 1] = NEXT_BYTE();  \
    m->registers[first] = NEXT_BYTE();      \
    tstates = 10;

#define MOV(to, from)                       \
    m->registers[to] = m->registers[from];  \
    tstates = 4;

#define MOV_r_m(to)                 \
    u16 from = FROM_HL();           \
    m->registers[to] = memory[from];\
    tstates = 7;

#define MOV_m_r(from)               \
    u16 to = FROM_HL();             \
    memory[to] = m->registers[from];\
    tstates = 7;

#define MVI(to)             \
    u8 data = NEXT_BYTE();  \
    m->registers[to] = data;\
    tstates = 7;

#define ORA(reg)                    \
    u8 with = m->registers[reg];    \
    LOGICAL_NOT_CMA(|);             \
    tstates = 4;

#define POP(reg)                            \
    m->registers[reg + 1] = memory[m->sp];  \
    m->sp++;                                \
    m->registers[reg] = memory[m->sp];      \
    m->sp++;                                \
    tstates = 10;

#define PUSH(reg)                               \
    memory[m->sp - 1] = m->registers[reg];      \
    memory[m->sp - 2] = m->registers[reg + 1];  \
    m->sp -= 2;                                 \
    tstates = 12;

#define RST(addr)                               \
    memory[m->sp - 1] = (m->pc & 0xff00) >> 8;  \
    memory[m->sp - 2] = m->pc & 0x00ff;         \
    m->sp -= 2;                                 \
    m->pc = addr;                               \
    tstates = 12;

#define SBB(reg)                                    \
    u8 by = m->registers[reg] + GET_FLAG(FLG_C);    \
    SUB();                                          \
    tstates = 4;

#define STAX(first)                         \
    u16 to = FROM_PAIR(first, first + 1);   \
    memory[to] = m->registers[REG_A];       \
    tstates = 7;

#define SUB_r(reg)              \
    u8 by = m->registers[reg];  \
    SUB();                      \
    tstates = 4;

#define XRA(reg)                    \
    u8 with = m->registers[reg];    \
    LOGICAL_NOT_CMA(^);             \
    tstates = 4;

#define WARN_NOT_IMPLEMENTED(ins)                   \
    pwarn("Instruction not implemented : " #ins);

void run(Machine *m, u8 *memory, u8 step){	
    u8 opcode;
    u8 tstates = 0;
    while((opcode = NEXT_BYTE()) != 0x76){
        switch(opcode){
            case 0xCE: // ACI Data
                {
                    u8 with1 = NEXT_BYTE(), with2 = GET_FLAG(FLG_C);
                    ADD2();
                    tstates = 7;
                    break;
                }
            case 0x8F: // ADC A
                {
                    ADC(REG_A);
                    break;
                }
            case 0x88: // ADC B
                {
                    ADC(REG_B);
                    break;
                }
            case 0x89: // ADC C
                {
                    ADC(REG_C);
                    break;
                }
            case 0x8A: // ADC D
                {
                    ADC(REG_D);
                    break;
                }
            case 0x8B: // ADC E
                {
                    ADC(REG_E);
                    break;
                }
            case 0x8C: // ADC H
                {
                    ADC(REG_H);
                    break;
                }
            case 0x8D: // ADC L
                {
                    ADC(REG_L);
                    break;
                }
            case 0x8E: // ADC M
                {
                    u8 with1 = memory[FROM_HL()], with2 = GET_FLAG(FLG_C);
                    ADD2();
                    tstates = 7;
                    break;
                }
            case 0x87: // ADD A
                {
                    ADD_R(REG_A);
                    break;
                }
            case 0x80: // ADD B
                {
                    ADD_R(REG_B);
                    break;
                }
            case 0x81: // ADD C
                {
                    ADD_R(REG_C);
                    break;
                }
            case 0x82: // ADD D 
                {
                    ADD_R(REG_D);
                    break;
                }
            case 0x83: // ADD E 
                {
                    ADD_R(REG_E);
                    break;
                }
            case 0x84: // ADD H
                {
                    ADD_R(REG_H);
                    break;
                }
            case 0x85: // ADD L
                {
                    ADD_R(REG_L);
                    break;
                }
            case 0x86: // ADD M
                {
                    u8 with = memory[FROM_HL()];
                    ADD();
                    tstates = 7;
                    break;
                }
            case 0xC6: // ADI Data
                {
                    u8 with = NEXT_BYTE();
                    ADD();
                    tstates = 7;
                    break;
                }
            case 0xA7: // ANA A
                {
                    ANA(REG_A);
                    break;
                }
            case 0xA0: // ANA B
                {
                    ANA(REG_B);
                    break;
                }
            case 0xA1: // ANA C
                {
                    ANA(REG_C);
                    break;
                }
            case 0xA2: // ANA D
                {
                    ANA(REG_D);
                    break;
                }
            case 0xA3: // ANA E
                {
                    ANA(REG_E);
                    break;
                }
            case 0xA4: // ANA H
                {
                    ANA(REG_H);
                    break;
                }
            case 0xA5: // ANA L
                {
                    ANA(REG_L);
                    break;
                }
            case 0xA6: // ANA M
                {
                    u8 with = memory[FROM_HL()];
                    LOGICAL_NOT_CMA(&);
                    SET_FLAG(FLG_A);
                    tstates = 7;
                    break;
                }
            case 0xE6: // ANI Data 
                {
                    u8 with = NEXT_BYTE();
                    LOGICAL_NOT_CMA(&);
                    SET_FLAG(FLG_A);
                    tstates = 7;
                    break;
                }
            case 0xCD: // CALL Label
                {
                    CALL_ON(1);
                    break;
                }
            case 0xDC: // CC Label
                {
                    CALL_ON(GET_FLAG(FLG_C));
                    break;
                }
            case 0xFC: // CM Label
                {
                    CALL_ON(GET_FLAG(FLG_S));
                    break;
                }
            case 0x2F: // CMA 
                {
                    m->registers[REG_A] = ~m->registers[REG_A];
                    tstates = 4;
                    break;
                }
            case 0x3F: // CMC
                {
                    CHANGE_FLAG(FLG_C, !(GET_FLAG(FLG_C)));
                    tstates = 4;
                    break;
                }
            case 0xBF: // CMP A
                {
                    CMP(REG_A);
                    break;
                }
            case 0xB8: // CMP B
                {
                    CMP(REG_B);
                    break;
                }
            case 0xB9: // CMP C
                {
                    CMP(REG_C);
                    break;
                }
            case 0xBA: // CMP D
                {
                    CMP(REG_D);
                    break;
                }
            case 0xBB: // CMP E
                {
                    CMP(REG_E);
                    break;
                }
            case 0xBC: // CMP H
                {
                    CMP(REG_H);
                    break;
                }
            case 0xBD: // CMP L
                {
                    CMP(REG_L);
                    break;
                }
            case 0xBE: // CMP M
                {
                    u8 bak = m->registers[REG_A];
                    u8 by = memory[FROM_HL()] + 1;
                    SUB();
                    m->registers[REG_A] = bak;
                    tstates = 7;
                    break;
                }
            case 0xD4: // CNC Label
                {
                    CALL_ON(!GET_FLAG(FLG_C));
                    break;
                }
            case 0xC4: // CNZ Label
                {
                    CALL_ON(!GET_FLAG(FLG_Z));
                    break;
                }
            case 0xF4: // CP Label
                {
                    CALL_ON(!GET_FLAG(FLG_S));
                    break;
                }
            case 0xEC: // CPE Label
                {
                    CALL_ON(GET_FLAG(FLG_P));
                    break;
                }
            case 0xFE: // CPI Data
                {
                    u8 bak = m->registers[REG_A];
                    u8 by = NEXT_BYTE();
                    SUB();
                    m->registers[REG_A] = bak;
                    tstates = 7;
                    break;
                }
            case 0xE4: // CPO Label
                {
                    CALL_ON(!GET_FLAG(FLG_P));
                    break;
                }
            case 0xCC: // CZ Label
                {
                    CALL_ON(GET_FLAG(FLG_Z));
                    break;
                }
            case 0x27 : // DAA
                {
                    u8 low = m->registers[REG_A] & 0x0f, high = m->registers[REG_A] & 0xf0;
                    u8 with = 0;
                    if(low > 9 || GET_FLAG(FLG_A))
                        with |= 0x06;
                    if(high > 9 || GET_FLAG(FLG_C))
                        with |= 0x60;
                    ADD();
                    tstates = 4;
                    break;
                }
            case 0x09: // DAD B
                {
                    DAD_R(REG_B);
                    break;
                }
            case 0x19: // DAD D 
                {
                    DAD_R(REG_D);
                    break;
                }
            case 0x29: // DAD H
                {
                    DAD_R(REG_H);
                    break;
                }
            case 0x39: // DAD SP
                {
                    u16 with = m->sp;
                    DAD();
                    break;
                }
            case 0x3D: // DCR A
                {
                    DCR(REG_A);
                    break;
                }
            case 0x05: // DCR B
                {
                    DCR(REG_B);
                    break;
                }
            case 0x0D: // DCR C
                {
                    DCR(REG_C);
                    break;
                }
            case 0x15: // DCR D
                {
                    DCR(REG_D);
                    break;
                }
            case 0x1D: // DCR E
                {
                    DCR(REG_E);
                    break;
                }
            case 0x25: // DCR H
                {
                    DCR(REG_H);
                    break;
                }
            case 0x2D: // DCR L
                {
                    DCR(REG_L);
                    break;
                }
            case 0x35: // DCR M
                {
                    u16 res = memory[FROM_HL()] - 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    INIT_FLG_P(res);
                    INIT_FLG_A(memory[FROM_HL()], -1);
                    memory[FROM_HL()] = res & 0xff;
                    tstates = 10;
                    break;
                }
            case 0x0B: // DCX B
                {
                    DCX(REG_B);
                    break;
                }
            case 0x1B: // DCX D
                {
                    DCX(REG_D);
                    break;
                }
            case 0x2B: // DCX H
                {
                    DCX(REG_H);
                    break;
                }
            case 0x3B: // DCX SP
                {
                    m->sp--;
                    tstates = 6;
                    break;
                }
            case 0xF3: // DI
                {
                    WARN_NOT_IMPLEMENTED(DI);
                    break;
                }
            case 0xFB: // EI
                {
                    WARN_NOT_IMPLEMENTED(EI);
                    break;
                }
            case 0x76: // HLT
                {
                    m->isbroken = 0;
                    tstates = 5;
                    return;
                }
            case 0xDB: // IN Port-Address
                {
                    u8 addr = NEXT_BYTE();
                    u32 val;
                    pblue("\n[in:0x%x] ", addr);
                    scanf("%x", &val);
                    m->registers[REG_A] = (u8)val;
                    tstates = 10;
                    break;
                }
            case 0x3C: // INR A
                {
                    INR(REG_A);
                    break;
                }
            case 0x04: // INR B
                {
                    INR(REG_B);
                    break;
                }
            case 0x0C: // INR C
                {
                    INR(REG_C);
                    break;
                }
            case 0x14: // INR D
                {
                    INR(REG_D);
                    break;
                }
            case 0x1C: // INR E
                {
                    INR(REG_E);
                    break;
                }
            case 0x24: // INR H
                {
                    INR(REG_H);
                    break;
                }
            case 0x2C: // INR L
                {
                    INR(REG_L);
                    break;
                }
            case 0x34: // INR M
                {
                    u16 res = memory[FROM_HL()] + 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    INIT_FLG_P(res);
                    INIT_FLG_A(memory[FROM_HL()], 1);
                    memory[FROM_HL()] = res & 0xff;
                    tstates = 10;
                    break;
                }
            case 0x03: // INX B
                {
                    INX(REG_B);
                    break;
                }
            case 0x13: // INX D 
                {
                    INX(REG_D);
                    break;
                }
            case 0x23: // INX H
                {
                    INX(REG_H);
                    break;
                }
            case 0x33: // INX SP
                {
                    m->sp++;
                    tstates = 6;
                    break;
                }
            case 0xDA: // JC Label
                {
                    JMP_ON(GET_FLAG(FLG_C));
                    break;
                }
            case 0xFA: // JM Label
                {
                    JMP_ON(GET_FLAG(FLG_S));
                    break;
                }
            case 0xC3: // JMP Label
                {
                    JMP_ON(1);
                    break;
                }
            case 0xD2: // JNC Label
                {
                    JMP_ON(!GET_FLAG(FLG_C));
                    break;
                }
            case 0xC2: // JNZ Label
                {
                    JMP_ON(!GET_FLAG(FLG_Z));
                    break;
                }
            case 0xF2: // JP Label
                {
                    JMP_ON(!GET_FLAG(FLG_S));
                    break;
                }
            case 0xEA: // JPE Label
                {
                    JMP_ON(GET_FLAG(FLG_P));
                    break;
                }
            case 0xE2: // JPO Label
                {
                    JMP_ON(!GET_FLAG(FLG_P));
                    break;
                }
            case 0xCA: // JZ Label
                {
                    JMP_ON(GET_FLAG(FLG_Z));
                    break;
                }
            case 0x3A: // LDA Address
                {
                    m->registers[REG_A] = memory[NEXT_DWORD()];
                    tstates = 13;
                    break;
                }
            case 0x0A: // LDAX B
                {
                    LDAX(REG_B);
                    break;
                }
            case 0x1A: // LDAX D
                {
                    LDAX(REG_D);
                    break;
                }
            case 0x2A: // LHLD Address
                {
                    u16 addr = NEXT_DWORD();
                    m->registers[REG_L] = memory[addr];
                    m->registers[REG_H] = memory[addr + 1];
                    tstates = 16;
                    break;
                }
            case 0x01: // LXI B
                {
                    LXI(REG_B);
                    break;
                }
            case 0x11: // LXI D
                {
                    LXI(REG_D);
                    break;
                }
            case 0x21: // LXI H
                {
                    LXI(REG_H);
                    break;
                }
            case 0x31: // LXI SP
                {
                    m->sp = NEXT_DWORD();
                    tstates = 10;
                    break;
                }
            case 0x7F: // MOV A, A
                {
                    MOV(REG_A, REG_A);
                    break;
                }
            case 0x78: // MOV A, B
                {
                    MOV(REG_A, REG_B);
                    break;
                }
            case 0x79: // MOV A, C
                {
                    MOV(REG_A, REG_C);
                    break;
                }
            case 0x7A: // MOV A, D
                {
                    MOV(REG_A, REG_D);
                    break;
                }
            case 0x7B: // MOV A, E
                {
                    MOV(REG_A, REG_E);
                    break;
                }
            case 0x7C: // MOV A, H
                {
                    MOV(REG_A, REG_H);
                    break;
                }
            case 0x7D: // MOV A, L
                {
                    MOV(REG_A, REG_L);
                    break;
                }
            case 0x7E: // MOV A, M 
                {
                    MOV_r_m(REG_A);
                    break;
                }
            case 0x47: // MOV B, A
                {
                    MOV(REG_B, REG_A);
                    break;
                }
            case 0x40: // MOV B, B
                {
                    MOV(REG_B, REG_B);
                    break;
                }
            case 0x41: // MOV B, C
                {
                    MOV(REG_B, REG_C);
                    break;
                }
            case 0x42: // MOV B, D
                {
                    MOV(REG_B, REG_D);
                    break;
                }
            case 0x43: // MOV B, E
                {
                    MOV(REG_B, REG_E);
                    break;
                }
            case 0x44: // MOV B, H
                {
                    MOV(REG_B, REG_H);
                    break;
                }
            case 0x45: // MOV B, L
                {
                    MOV(REG_B, REG_L);
                    break;
                }
            case 0x46: // MOV B, M
                {
                    MOV_r_m(REG_B);
                    break;
                }
            case 0x4F: // MOV C, A
                {
                    MOV(REG_C, REG_A);
                    break;
                }
            case 0x48: // MOV C, B
                {
                    MOV(REG_C, REG_B);
                    break;
                }
            case 0x49: // MOV C, C 
                {
                    MOV(REG_C, REG_C);
                    break;
                }
            case 0x4A: // MOV C, D
                {
                    MOV(REG_C, REG_D);
                    break;
                }
            case 0x4B: // MOV C, E
                {
                    MOV(REG_C, REG_E);
                    break;
                }
            case 0x4C: // MOV C, H
                {
                    MOV(REG_C, REG_H);
                    break;
                }
            case 0x4D: // MOV C, L
                {
                    MOV(REG_C, REG_L);
                    break;
                }
            case 0x4E: // MOV C, M
                {
                    MOV_r_m(REG_C);
                    break;
                }
            case 0x57: // MOV D, A
                {
                    MOV(REG_D, REG_A);
                    break;
                }
            case 0x50: // MOV D, B
                {
                    MOV(REG_D, REG_B);
                    break;
                }
            case 0x51: // MOV D, C
                {
                    MOV(REG_D, REG_C);
                    break;
                }
            case 0x52: // MOV D, D
                {
                    MOV(REG_D, REG_D);
                    break;
                }
            case 0x53: // MOV D, E
                {
                    MOV(REG_D, REG_E);
                    break;
                }
            case 0x54: // MOV D, H
                {
                    MOV(REG_D, REG_H);
                    break;
                }
            case 0x55: // MOV D, L
                {
                    MOV(REG_D, REG_L);
                    break;
                }
            case 0x56: // MOV D, M
                {
                    MOV_r_m(REG_D);
                    break;
                }
            case 0x5F: // MOV E, A
                {
                    MOV(REG_E, REG_A);
                    break;
                }
            case 0x58: // MOV E, B
                {
                    MOV(REG_E, REG_B);
                    break;
                }
            case 0x59: // MOV E, C
                {
                    MOV(REG_E, REG_C);
                    break;
                }
            case 0x5A: // MOV E, D
                {
                    MOV(REG_E, REG_D);
                    break;
                }
            case 0x5B: // MOV E, E
                {
                    MOV(REG_E, REG_E);
                    break;
                }
            case 0x5C: // MOV E, H
                {
                    MOV(REG_E, REG_H);
                    break;
                }
            case 0x5D: // MOV E, L
                {
                    MOV(REG_E, REG_L);
                    break;
                }
            case 0x5E: // MOV E, M
                {
                    MOV_r_m(REG_E);
                    break;
                }
            case 0x67: // MOV H, A
                {
                    MOV(REG_H, REG_A);
                    break;
                }
            case 0x60: // MOV H, B
                {
                    MOV(REG_H, REG_B);
                    break;
                }
            case 0x61: // MOV H, C
                {
                    MOV(REG_H, REG_C);
                    break;
                }
            case 0x62: // MOV H, D
                {
                    MOV(REG_H, REG_D);
                    break;
                }
            case 0x63: // MOV H, E
                {
                    MOV(REG_H, REG_E);
                    break;
                }
            case 0x64: // MOV H, H
                {
                    MOV(REG_H, REG_H);
                    break;
                }
            case 0x65: // MOV H, L
                {
                    MOV(REG_H, REG_L);
                    break;
                }
            case 0x66: // MOV H, M
                {
                    MOV_r_m(REG_H);
                    break;
                }
            case 0x6F: // MOV L, A
                {
                    MOV(REG_L, REG_A);
                    break;
                }
            case 0x68: // MOV L, B 
                {
                    MOV(REG_L, REG_B);
                    break;
                }
            case 0x69: // MOV L, C
                {
                    MOV(REG_L, REG_C);
                    break;
                }
            case 0x6A: // MOV L, D
                {
                    MOV(REG_L, REG_D);
                    break;
                }
            case 0x6B: // MOV L, E
                {
                    MOV(REG_L, REG_E);
                    break;
                }
            case 0x6C: // MOV L, H
                {
                    MOV(REG_L, REG_H);
                    break;
                }
            case 0x6D: // MOV L, L
                {
                    MOV(REG_L, REG_L);
                    break;
                }
            case 0x6E: // MOV L, M
                {
                    MOV_r_m(REG_L);
                    break;
                }
            case 0x77: // MOV M, A
                {
                    MOV_m_r(REG_A);
                    break;
                }
            case 0x70: // MOV M, B
                {
                    MOV_m_r(REG_B);
                    break;
                }
            case 0x71: // MOV M, C
                {
                    MOV_m_r(REG_C);
                    break;
                }
            case 0x72: // MOV M, D
                {
                    MOV_m_r(REG_D);
                    break;
                }
            case 0x73: // MOV M, E
                {
                    MOV_m_r(REG_E);
                    break;
                }
            case 0x74: // MOV M, H
                {
                    MOV_m_r(REG_H);
                    break;
                }
            case 0x75: // MOV M, L
                {
                    MOV_m_r(REG_L);
                    break;
                }
            case 0x3E: // MVI A, Data
                {
                    MVI(REG_A);
                    break;
                }
            case 0x06: // MVI B, Data
                {
                    MVI(REG_B);
                    break;
                }
            case 0x0E: // MVI C, Data
                {
                    MVI(REG_C);
                    break;
                }
            case 0x16: // MVI D, Data
                {
                    MVI(REG_D);
                    break;
                }
            case 0x1E: // MVI E, Data
                {
                    MVI(REG_E);
                    break;
                }
            case 0x26: // MVI H, Data
                {
                    MVI(REG_H);
                    break;
                }
            case 0x2E: // MVI L, Data
                {
                    MVI(REG_L);
                    break;
                }
            case 0x36: // MVI M, Data
                {
                    u16 to = FROM_HL();
                    memory[to] = NEXT_BYTE();
                    tstates = 10;
                    break;
                }
            case 0x00: // NOP
                {
                    tstates = 4;
                    break;
                }
            case 0xB7: // ORA A
                {
                    ORA(REG_A);
                    break;
                }
            case 0xB0: // ORA B
                {
                    ORA(REG_B);
                    break;
                }
            case 0xB1: // ORA C
                {
                    ORA(REG_C);
                    break;
                }
            case 0xB2: // ORA D
                {
                    ORA(REG_D);
                    break;
                }
            case 0xB3: // ORA E
                {
                    ORA(REG_E);
                    break;
                }
            case 0xB4: // ORA H
                {
                    ORA(REG_H);
                    break;
                }
            case 0xB5: // ORA L
                {
                    ORA(REG_L);
                    break;
                }
            case 0xB6: // ORA M
                {
                    u8 with = memory[FROM_HL()];
                    LOGICAL_NOT_CMA(|);
                    tstates = 7;
                    break;
                }
            case 0xF6: // ORI Data
                {
                    u8 with = NEXT_BYTE();
                    LOGICAL_NOT_CMA(|);
                    tstates = 7;
                    break;
                }
            case 0xD3: // OUT Port-Address
                {
                    u8 addr = NEXT_BYTE();
                    tstates = 7;
                    if(!m->issilent){
                        pylw("\n[out:0x%x]", addr);
                        printf(" 0x%x", m->registers[REG_A]);
                        fflush(stdout);
                        tstates = 10;
                    }
                    break;
                }
            case 0xE9: // PCHL
                {
                    m->pc = m->registers[REG_L];
                    m->pc |= (m->registers[REG_H] << 8);
                    tstates = 6;
                    break;
                }
            case 0xC1: // POP B
                {
                    POP(REG_B);
                    break;
                }
            case 0xD1: // POP D
                {
                    POP(REG_D);
                    break;
                }
            case 0xE1: // POP H
                {
                    POP(REG_H);
                    break;
                }
            case 0xF1: // POP PSW
                {
                    m->registers[REG_FL] = memory[m->sp];
                    m->sp++;
                    m->registers[REG_A] = memory[m->sp];
                    m->sp++;
                    tstates = 10;
                    break;
                }
            case 0xC5: // PUSH B
                {
                    PUSH(REG_B);
                    break;
                }
            case 0xD5: // PUSH D
                {
                    PUSH(REG_D);
                    break;
                }
            case 0xE5: // PUSH H
                {
                    PUSH(REG_H);
                    break;
                }
            case 0xF5: // PUSH PSW
                {
                    memory[m->sp - 1] = m->registers[REG_A];
                    memory[m->sp - 2] = m->registers[REG_FL];
                    m->sp -= 2;
                    tstates = 12;
                    break;
                }
            case 0x17: // RAL
                {
                    u8 d7 = m->registers[REG_A] >> 7;
                    u8 d0 = GET_FLAG(FLG_C);
                    CHANGE_FLAG(FLG_C, d7); // Set/reset the carry
                    m->registers[REG_A] <<= 1;
                    m->registers[REG_A] |= d0;
                    tstates = 4;
                    break;
                }
            case 0x1F: // RAR
                {
                    u8 d0 = m->registers[REG_A] & 1;
                    u8 d7 = GET_FLAG(FLG_C);
                    CHANGE_FLAG(FLG_C, d0); // Set/reset the carry
                    m->registers[REG_A] >>= 1;
                    m->registers[REG_A] |= (d7 << 7);
                    tstates = 4;
                    break;
                }
            case 0xD8: // RC
                {
                    RET_ON(GET_FLAG(FLG_C));
                    break;
                }
            case 0xC9: // RET
                {
                    RET_ON(1);
                    break;
                }
            case 0x20: // RIM
                {
                    WARN_NOT_IMPLEMENTED(RIM);
                    break;
                }
            case 0x07: // RLC 
                {
                    u8 d7 = m->registers[REG_A] >> 7;
                    CHANGE_FLAG(FLG_C, d7); // Set/reset the carry
                    m->registers[REG_A] <<= 1;
                    m->registers[REG_A] |= d7;
                    tstates = 4;
                    break;
                }
            case 0xF8: // RM
                {
                    RET_ON(GET_FLAG(FLG_S));
                    break;
                }
            case 0xD0: // RNC
                {
                    RET_ON(!GET_FLAG(FLG_C));
                    break;
                }
            case 0xC0: // RNZ
                {
                    RET_ON(!GET_FLAG(FLG_Z));
                    break;
                }
            case 0xF0: // RP
                {
                    RET_ON(!GET_FLAG(FLG_S));
                    break;
                }
            case 0xE8: // RPE
                {
                    RET_ON(GET_FLAG(FLG_P));
                    break;
                }
            case 0xE0: // RPO
                {
                    RET_ON(!GET_FLAG(FLG_P));
                    break;
                }
            case 0x0F: // RRC
                {
                    u8 d0 = m->registers[REG_A] & 1;
                    CHANGE_FLAG(FLG_C, d0); // Set/reset the carry
                    m->registers[REG_A] >>= 1;
                    m->registers[REG_A] |= (d0 << 7);
                    tstates = 4;
                    break;
                }
            case 0xC7: // RST 0
                {
                    RST(0x0000);
                    break;
                }
            case 0xCF: // RST 1
                {
                    RST(0x0008);
                    break;
                }
            case 0xD7: // RST 2
                {
                    RST(0x0010);
                    break;
                }
            case 0xDF: // RST 3
                {
                    RST(0x0018);
                    break;
                }
            case 0xE7: // RST 4
                {
                    RST(0x0020);
                    break;
                }
            case 0xEF: // RST 5
                {
                    RST(0x0028);
                    break;
                }
            case 0xF7: // RST 6
                {
                    RST(0x0030);
                    break;
                }
            case 0xFF: // RST 7
                {
                    RST(0x0038);
                    break;
                }
            case 0xC8: // RZ
                {
                    RET_ON(GET_FLAG(FLG_Z));
                    break;
                }
            case 0x9F: // SBB A
                {
                    SBB(REG_A);
                    break;
                }
            case 0x98: // SBB B
                {
                    SBB(REG_B);
                    break;
                }
            case 0x99: // SBB C
                {
                    SBB(REG_C);
                    break;
                }
            case 0x9A: // SBB D
                {
                    SBB(REG_D);
                    break;
                }
            case 0x9B: // SBB E
                {
                    SBB(REG_E);
                    break;
                }
            case 0x9C: // SBB H
                {
                    SBB(REG_H);
                    break;
                }
            case 0x9D: // SBB L
                {
                    SBB(REG_L);
                    break;
                }
            case 0x9E: // SBB M
                {
                    u8 by = memory[FROM_HL()] + GET_FLAG(FLG_C);
                    SUB();
                    tstates = 7;
                    break;
                }
            case 0xDE: // SBI Data
                {
                    u8 by = NEXT_BYTE() + GET_FLAG(FLG_C);
                    SUB();
                    tstates = 7;
                    break;
                }
            case 0x22: // SHLD Address
                {
                    u16 addr = NEXT_DWORD();
                    memory[addr] = m->registers[REG_L];
                    memory[addr + 1] = m->registers[REG_H];
                    tstates = 16;
                    break;
                }
            case 0x30: // SIM
                {
                    WARN_NOT_IMPLEMENTED(SIM);
                    break;
                }
            case 0xF9: // SPHL
                {
                    m->sp = FROM_HL();
                    tstates = 6;
                    break;
                }
            case 0x32: // STA Address
                {
                    u16 to = NEXT_DWORD();
                    memory[to] = m->registers[REG_A];
                    tstates = 13;
                    break;
                }
            case 0x02: // STAX B
                {
                    STAX(REG_B);
                    break;
                }
            case 0x12: // STAX D
                {
                    STAX(REG_D);
                    break;
                }
            case 0x37: // STC
                {
                    SET_FLAG(FLG_C);
                    break;
                }
            case 0x97: // SUB A
                {
                    SUB_r(REG_A);
                    break;
                }
            case 0x90: // SUB B
                {
                    SUB_r(REG_B);
                    break;
                }
            case 0x91: // SUB C 
                {
                    SUB_r(REG_C);
                    break;
                }
            case 0x92: // SUB D
                {
                    SUB_r(REG_D);
                    break;
                }
            case 0x93: // SUB E
                {
                    SUB_r(REG_E);
                    break;
                }
            case 0x94: // SUB H 
                {
                    SUB_r(REG_H);
                    break;
                }
            case 0x95: // SUB L
                {
                    SUB_r(REG_L);
                    break;
                }
            case 0x96: // SUB M
                {
                    u8 by = memory[FROM_HL()];
                    SUB();
                    tstates = 7;
                    break;
                }
            case 0xD6: // SUI Data
                {
                    u8 by = NEXT_BYTE();
                    SUB();
                    tstates = 7;
                    break;
                }
            case 0xEB: // XCHG
                {
                    u8 td = m->registers[REG_D];
                    u8 te = m->registers[REG_E];
                    m->registers[REG_D] = m->registers[REG_H];
                    m->registers[REG_E] = m->registers[REG_L];
                    m->registers[REG_H] = td;
                    m->registers[REG_L] = te;
                    tstates = 4;
                    break;
                }
            case 0xAF: // XRA A
                {
                    XRA(REG_A);
                    break;
                }
            case 0xA8: // XRA B
                {
                    XRA(REG_B);
                    break;
                }
            case 0xA9: // XRA C
                {
                    XRA(REG_C);
                    break;
                }
            case 0xAA: // XRA D
                {
                    XRA(REG_D);
                    break;
                }
            case 0xAB: // XRA E
                {
                    XRA(REG_E);
                    break;
                }
            case 0xAC: // XRA H
                {
                    XRA(REG_H);
                    break;
                }
            case 0xAD: // XRA L
                {
                    XRA(REG_L);
                    break;
                }
            case 0xAE: // XRA M
                {
                    u8 with = memory[FROM_HL()];
                    LOGICAL_NOT_CMA(^);
                    tstates = 7;
                    break;
                }
            case 0xEE: // XRI Data
                {
                    u8 with = NEXT_BYTE();
                    LOGICAL_NOT_CMA(^);
                    tstates = 7;
                    break;
                }
            case 0xE3: // XTHL
                {
                    u8 td = memory[m->sp + 1];
                    u8 te = memory[m->sp];
                    memory[m->sp + 1] = m->registers[REG_H];
                    memory[m->sp] = m->registers[REG_L];
                    m->registers[REG_H] = td;
                    m->registers[REG_L] = te;
                    tstates = 16;
                    break;
                }
        }
        if(m->sleepfor.tv_nsec > 0){
            struct timespec timetosleep = m->sleepfor;
            timetosleep.tv_nsec *= tstates;
            nanosleep(&timetosleep, NULL);
        }
        if(machine_on_breakpoint(m, memory, step))
            return;
    }
    m->isbroken = 0;
}

#endif
