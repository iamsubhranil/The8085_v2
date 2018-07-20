#include <stdio.h>

#include "vm.h"
#include "bytecode.h"
#include "display.h"

#define GET_FLAG(x)             ((m->registers[REG_FL] >> x) & 1)
void print_machine(Machine *m){
    char regs[] = {'A', 'B', 'C', 'D', 'E', 'H', 'L'};
    pgrn("\n[Registers]\t");
    for(u8 i = 0;i < 7;i++)
        pcyn("%4c\t", regs[i], " ");
    pmgn("\n          \t");
    for(u8 i = 0;i < 7;i++)
        phmgn("", "0x%02x\t", m->registers[i]);

    pgrn("\n[FLAGS] ");
    char flags[] = {'S', 'Z', ' ', 'A', ' ', 'P', ' ', 'C'};
    for(u8 i = 0;i < 8;i++)
        pcyn("%c ", flags[i]);
    pgrn("\n        ");
    for(int i = 7;i >= 0;i--)
        phred("", "%d ", GET_FLAG(i));
    
    phblue("\n[PC] ", "0x%0x", m->pc);
    phylw("\n[SP] ", "0x%0x", m->sp);
}

void run(Machine *m, u8 *memory){
    #define NEXT_BYTE()         memory[m->pc++]
    #define NEXT_DWORD()        ((u16)NEXT_BYTE() | ((u16)NEXT_BYTE() << 8))
    
    #define FROM_PAIR(x, y)     (((u16)m->registers[x] << 8) | m->registers[y])
    #define FROM_HL()           FROM_PAIR(5, 6)
    
    #define SET_FLAG(x)         (m->registers[REG_FL] |= (1 << x))
    #define RESET_FLAG(x)       (m->registers[REG_FL] &= ~(1 << x))
   
    #define INIT_FLG_C(res)     \
        if(res > 0xff)          \
            SET_FLAG(FLG_C);    \
        else                    \
            RESET_FLAG(FLG_C);

    #define INIT_FLG_Z(res)     \
        if((res & 0xff) == 0)   \
            SET_FLAG(FLG_Z);    \
        else                    \
            RESET_FLAG(FLG_Z);

    #define INIT_FLG_S(res)     \
        if((res >> 7) & 1)      \
            SET_FLAG(FLG_S);    \
        else                    \
            RESET_FLAG(FLG_S);

    #define INIT_FLAGS(res)     \
        INIT_FLG_C(res)         \
        INIT_FLG_Z(res)         \
        INIT_FLG_S(res) 

    #define ADD()                                           \
        u16 res = with + m->registers[REG_A];               \
        INIT_FLAGS(res);                                    \
        m->registers[REG_A] = res & 0xff;
   
    #define SUB()               \
        u8 with = ~by + 1;      \
        ADD();                  \
        if(GET_FLAG(FLG_C))     \
            RESET_FLAG(FLG_C);  \
        else                    \
            SET_FLAG(FLG_C);

    #define LOGICAL(op)   \
        m->registers[REG_A] = m->registers[REG_A] op with;

    #define LOGICAL_NOT_CMA(op)         \
        LOGICAL(op)                     \
        RESET_FLAG(FLG_C);              \
        INIT_FLG_S(m->registers[REG_A]) \
        INIT_FLG_Z(m->registers[REG_A])

    #define JMP_ON(cond)            \
        u16 addr = NEXT_DWORD();    \
        if(cond)                    \
            m->pc = addr;           \
        break;

    #define CALL_ON(cond)                           \
        u16 addr = NEXT_DWORD();                    \
        if(cond){                                   \
            memory[m->sp] = (m->pc & 0xff00) >> 8;  \
            memory[m->sp - 1] = m->pc & 0x00ff;     \
            m->sp -= 2;                             \
            m->pc = addr;                           \
        }                                           \
        break;

    #define RET_ON(cond)                \
        if(cond){                       \
            m->pc = memory[m->sp] << 8; \
            m->pc |= memory[m->sp + 1]; \
            m->sp += 2;                 \
        }                               \
        break;

    Bytecode opcode;
    while((opcode = (Bytecode)NEXT_BYTE()) != HLT){
        print_machine(m);
        switch(opcode){
            case MOV:
                {
                    u8 to = NEXT_BYTE();
                    u8 from = NEXT_BYTE();
                    m->registers[to] = m->registers[from];
                    break;
                }
            case MOV_R:
                {
                    u8 to = NEXT_BYTE();
                    u16 from = FROM_HL();
                    m->registers[to] = memory[from];
                    break;
                }
            case MOV_M:
                {
                    u8 from = NEXT_BYTE();
                    u16 to = FROM_HL();
                    memory[to] = m->registers[from];
                    break;
                }
            case MVI:
                {
                    u8 to = NEXT_BYTE();
                    u8 data = NEXT_BYTE();
                    m->registers[to] = data;
                    break;
                }
            case MVI_M:
                {
                    u16 to = FROM_HL();
                    memory[to] = NEXT_BYTE();
                    break;
                }
            case LXI:
                {
                    u8 first = NEXT_BYTE();
                    m->registers[first + 1] = NEXT_BYTE();
                    m->registers[first] = NEXT_BYTE();
                    break;
                }
            case LXI_SP:
                {
                    m->sp = NEXT_DWORD();
                    break;
                }
            case LDA:
                {
                    m->registers[REG_A] = memory[NEXT_DWORD()];
                    break;
                }
            case LDAX:
                {
                    u8 first = NEXT_BYTE();
                    u16 from = FROM_PAIR(first, first + 1);
                    m->registers[REG_A] = memory[from];
                    break;
                }
            case STA:
                {
                    u16 to = NEXT_DWORD();
                    memory[to] = m->registers[REG_A];
                    break;
                }
            case STAX:
                {
                    u8 first = NEXT_BYTE();
                    u16 to = FROM_PAIR(first, first + 1);
                    memory[to] = m->registers[REG_A];
                    break;
                }
            case ADD:
                {
                    u8 with = m->registers[NEXT_BYTE()];
                    ADD();
                    break;
                }
            case ADD_M:
                {
                    u8 with = memory[FROM_HL()];
                    ADD();
                    break;
                }
            case ADI:
                {
                    u8 with = NEXT_BYTE();
                    ADD();
                    break;
                }
            case INR:
                {
                    u8 reg = NEXT_BYTE();
                    u16 res = m->registers[reg] + 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    m->registers[reg] = res & 0xff;
                    break;
                }
            case INR_M:
                {
                    u16 res = memory[FROM_HL()] + 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    memory[FROM_HL()] = res & 0xff;
                    break;
                }
            case INX:
                {
                    u8 first = NEXT_BYTE();
                    u16 res = FROM_PAIR(first, first + 1) + 1;
                    m->registers[first] = (res & 0xff00) >> 8;
                    m->registers[first + 1] = res & 0x00ff;
                    break;
                }
            case INX_SP:
                {
                    m->sp++;
                    break;
                }
            case SUB:
                {
                    u8 by = m->registers[NEXT_BYTE()];
                    SUB();
                    break;
                }
            case SUB_M:
                {
                    u8 by = memory[FROM_HL()];
                    SUB();
                    break;
                }
            case SUI:
                {
                    u8 by = NEXT_BYTE();
                    SUB();
                    break;
                }
            case DCR:
                {
                    u8 reg = NEXT_BYTE();
                    u16 res = m->registers[reg] - 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    m->registers[reg] = res & 0xff;
                    break;
                }
            case DCR_M:
                {
                    u16 res = memory[FROM_HL()] - 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    memory[FROM_HL()] = res & 0xff;
                    break;
                }
            case DCX:
                {
                    u8 first = NEXT_BYTE();
                    u16 res = FROM_PAIR(first, first + 1) - 1;
                    m->registers[first] = (res & 0xff00) >> 8;
                    m->registers[first + 1] = res & 0x00ff;
                    break;
                }
            case DCX_SP:
                {
                    m->sp--;
                    break;
                }
            case ANA:
                {
                    u8 with = m->registers[NEXT_BYTE()];
                    LOGICAL_NOT_CMA(&);
                    SET_FLAG(FLG_A);
                    break;
                }
            case ANA_M:
                {
                    u8 with = memory[FROM_HL()];
                    LOGICAL_NOT_CMA(&);
                    SET_FLAG(FLG_A);
                    break;
                }
            case ANI:
                {
                    u8 with = NEXT_BYTE();
                    LOGICAL_NOT_CMA(&);
                    SET_FLAG(FLG_A);
                    break;
                }
            case ORA:
                {
                    u8 with = m->registers[NEXT_BYTE()];
                    LOGICAL_NOT_CMA(|);
                    break;
                }
            case ORA_M:
                {
                    u8 with = memory[FROM_HL()];
                    LOGICAL_NOT_CMA(|);
                    break;
                }
            case ORI:
                {
                    u8 with = NEXT_BYTE();
                    LOGICAL_NOT_CMA(|);
                    break;
                }
            case XRA:
                {
                    u8 with = m->registers[NEXT_BYTE()];
                    LOGICAL_NOT_CMA(^);
                    break;
                }
            case XRA_M:
                {
                    u8 with = m->registers[NEXT_BYTE()];
                    LOGICAL_NOT_CMA(^);
                    break;
                }
            case XRI:
                {
                    u8 with = NEXT_BYTE();
                    LOGICAL_NOT_CMA(^);
                    break;
                }
            case CMA:
                {
                    m->registers[REG_A] = ~m->registers[REG_A];
                    break;
                }
            case CMP:
                {
                    u8 bak = m->registers[REG_A];
                    u8 by = m->registers[NEXT_BYTE()] + 1;
                    SUB();
                    m->registers[REG_A] = bak;
                    break;
                }
            case CMP_M:
                {
                    u8 bak = m->registers[REG_A];
                    u8 by = memory[FROM_HL()] + 1;
                    SUB();
                    m->registers[REG_A] = bak;
                    break;
                }
            case CPI:
                {
                    u8 bak = m->registers[REG_A];
                    u8 by = NEXT_BYTE();
                    SUB();
                    m->registers[REG_A] = bak;
                    break;
                }
            case RLC:
                {
                    u8 d7 = m->registers[REG_A] >> 7;
                    m->registers[REG_FL] |= (d7 << FLG_C); // Set/reset the carry
                    m->registers[REG_A] <<= 1;
                    m->registers[REG_A] |= d7;
                    break;
                }
            case RAL:
                {
                    u8 d7 = m->registers[REG_A] >> 7;
                    u8 d0 = GET_FLAG(FLG_C);
                    m->registers[REG_FL] |= (d7 << FLG_C); // Set/reset the carry
                    m->registers[REG_A] <<= 1;
                    m->registers[REG_A] |= d0;
                    break;
                }
            case RRC:
                {
                    u8 d0 = m->registers[REG_A] & 1;
                    m->registers[REG_FL] |= (d0 << FLG_C); // Set/reset the carry
                    m->registers[REG_A] >>= 1;
                    m->registers[REG_A] |= (d0 << 7);
                    break;
                }
            case RAR:
                {
                    u8 d0 = m->registers[REG_A] & 1;
                    u8 d7 = GET_FLAG(FLG_C);
                    m->registers[REG_FL] |= (d0 << FLG_C); // Set/reset the carry
                    m->registers[REG_A] >>= 1;
                    m->registers[REG_A] |= (d7 << 7);
                    break;
                }
            case JMP:
                {
                    JMP_ON(1);
                }
            case JC:
                {
                    JMP_ON(GET_FLAG(FLG_C));
                }
            case JNC:
                {
                    JMP_ON(!GET_FLAG(FLG_C));
                }
            case JZ:
                {
                    JMP_ON(GET_FLAG(FLG_Z));
                }
            case JNZ:
                {
                    JMP_ON(!GET_FLAG(FLG_Z));
                }
            case JP:
                {
                    JMP_ON(!GET_FLAG(FLG_S));
                }
            case JM:
                {
                    JMP_ON(GET_FLAG(FLG_S));
                }
            case CALL:
                {
                    CALL_ON(1);
                }
            case CC:
                {
                    CALL_ON(GET_FLAG(FLG_C));
                }
            case CNC:
                {
                    CALL_ON(!GET_FLAG(FLG_C));
                }
            case CZ:
                {
                    CALL_ON(GET_FLAG(FLG_Z));
                }
            case CNZ:
                {
                    CALL_ON(!GET_FLAG(FLG_Z));
                }
            case CM:
                {
                    CALL_ON(GET_FLAG(FLG_S));
                }
            case CP:
                {
                    CALL_ON(!GET_FLAG(FLG_S));
                }
            case RET:
                {
                    RET_ON(1);
                }
            case RC:
                {
                    RET_ON(GET_FLAG(FLG_C));
                }
            case RNC:
                {
                    RET_ON(!GET_FLAG(FLG_C));
                }
            case RZ:
                {
                    RET_ON(GET_FLAG(FLG_Z));
                }
            case RNZ:
                {
                    RET_ON(!GET_FLAG(FLG_Z));
                }
            case RM:
                {
                    RET_ON(GET_FLAG(FLG_S));
                }
            case RP:
                {
                    RET_ON(!GET_FLAG(FLG_S));
                }
            case IN:
                {
                    fflush(stdin);
                    m->registers[REG_A] = getchar();
                    break;
                }
            case OUT:
                {
                    printf("\n0x%x\n", m->registers[REG_A]);
                    fflush(stdout);
                    break;
                }
            case HLT:
                return;
            case NOP:
                break;
            default:
                break;
        }
    }
}
