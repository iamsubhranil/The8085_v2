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

    #define ADD2()                                          \
        u16 res = with1 + with2 + m->registers[REG_A];      \
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
    while((opcode = (Bytecode)NEXT_BYTE()) != BYTECODE_hlt){
        printf("\n");
        bytecode_disassemble_in_context(memory, m->pc - 1, m);
        switch(opcode){
            case BYTECODE_mov:
                {
                    u8 to = NEXT_BYTE();
                    u8 from = NEXT_BYTE();
                    m->registers[to] = m->registers[from];
                    break;
                }
            case BYTECODE_mov_R:
                {
                    u8 to = NEXT_BYTE();
                    u16 from = FROM_HL();
                    m->registers[to] = memory[from];
                    break;
                }
            case BYTECODE_mov_M:
                {
                    u8 from = NEXT_BYTE();
                    u16 to = FROM_HL();
                    memory[to] = m->registers[from];
                    break;
                }
            case BYTECODE_mvi:
                {
                    u8 to = NEXT_BYTE();
                    u8 data = NEXT_BYTE();
                    m->registers[to] = data;
                    break;
                }
            case BYTECODE_mvi_M:
                {
                    u16 to = FROM_HL();
                    memory[to] = NEXT_BYTE();
                    break;
                }
            case BYTECODE_lxi:
                {
                    u8 first = NEXT_BYTE();
                    m->registers[first + 1] = NEXT_BYTE();
                    m->registers[first] = NEXT_BYTE();
                    break;
                }
            case BYTECODE_lxi_SP:
                {
                    m->sp = NEXT_DWORD();
                    break;
                }
            case BYTECODE_lda:
                {
                    m->registers[REG_A] = memory[NEXT_DWORD()];
                    break;
                }
            case BYTECODE_ldax:
                {
                    u8 first = NEXT_BYTE();
                    u16 from = FROM_PAIR(first, first + 1);
                    m->registers[REG_A] = memory[from];
                    break;
                }
            case BYTECODE_sta:
                {
                    u16 to = NEXT_DWORD();
                    memory[to] = m->registers[REG_A];
                    break;
                }
            case BYTECODE_stax:
                {
                    u8 first = NEXT_BYTE();
                    u16 to = FROM_PAIR(first, first + 1);
                    memory[to] = m->registers[REG_A];
                    break;
                }
            case BYTECODE_aci:
                {
                    u8 with1 = NEXT_BYTE(), with2 = GET_FLAG(FLG_C);
                    ADD2();
                    break;
                }
            case BYTECODE_adc:
                {
                    u8 with1 = m->registers[NEXT_BYTE()], with2 = GET_FLAG(FLG_C);
                    ADD2();
                    break;
                }
            case BYTECODE_adc_M:
                {
                    u8 with1 = memory[FROM_HL()], with2 = GET_FLAG(FLG_C);
                    ADD2();
                    break;
                }
            case BYTECODE_add:
                {
                    u8 with = m->registers[NEXT_BYTE()];
                    ADD();
                    break;
                }
            case BYTECODE_add_M:
                {
                    u8 with = memory[FROM_HL()];
                    ADD();
                    break;
                }
            case BYTECODE_adi:
                {
                    u8 with = NEXT_BYTE();
                    ADD();
                    break;
                }
            case BYTECODE_inr:
                {
                    u8 reg = NEXT_BYTE();
                    u16 res = m->registers[reg] + 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    m->registers[reg] = res & 0xff;
                    break;
                }
            case BYTECODE_inr_M:
                {
                    u16 res = memory[FROM_HL()] + 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    memory[FROM_HL()] = res & 0xff;
                    break;
                }
            case BYTECODE_inx:
                {
                    u8 first = NEXT_BYTE();
                    u16 res = FROM_PAIR(first, first + 1) + 1;
                    m->registers[first] = (res & 0xff00) >> 8;
                    m->registers[first + 1] = res & 0x00ff;
                    break;
                }
            case BYTECODE_inx_SP:
                {
                    m->sp++;
                    break;
                }
            case BYTECODE_sub:
                {
                    u8 by = m->registers[NEXT_BYTE()];
                    SUB();
                    break;
                }
            case BYTECODE_sub_M:
                {
                    u8 by = memory[FROM_HL()];
                    SUB();
                    break;
                }
            case BYTECODE_sui:
                {
                    u8 by = NEXT_BYTE();
                    SUB();
                    break;
                }
            case BYTECODE_daa:
                {
                    u8 low = m->registers[REG_A] & 0x0f, high = m->registers[REG_A] & 0xf0;
                    u8 with = 0;
                    if(low > 9 || GET_FLAG(FLG_A))
                        with |= 0x06;
                    if(high > 9 || GET_FLAG(FLG_C))
                        with |= 0x60;
                    ADD();
                    break;
                }
            case BYTECODE_dcr:
                {
                    u8 reg = NEXT_BYTE();
                    u16 res = m->registers[reg] - 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    m->registers[reg] = res & 0xff;
                    break;
                }
            case BYTECODE_dcr_M:
                {
                    u16 res = memory[FROM_HL()] - 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    memory[FROM_HL()] = res & 0xff;
                    break;
                }
            case BYTECODE_dcx:
                {
                    u8 first = NEXT_BYTE();
                    u16 res = FROM_PAIR(first, first + 1) - 1;
                    m->registers[first] = (res & 0xff00) >> 8;
                    m->registers[first + 1] = res & 0x00ff;
                    break;
                }
            case BYTECODE_dcx_SP:
                {
                    m->sp--;
                    break;
                }
            case BYTECODE_ana:
                {
                    u8 with = m->registers[NEXT_BYTE()];
                    LOGICAL_NOT_CMA(&);
                    SET_FLAG(FLG_A);
                    break;
                }
            case BYTECODE_ana_M:
                {
                    u8 with = memory[FROM_HL()];
                    LOGICAL_NOT_CMA(&);
                    SET_FLAG(FLG_A);
                    break;
                }
            case BYTECODE_ani:
                {
                    u8 with = NEXT_BYTE();
                    LOGICAL_NOT_CMA(&);
                    SET_FLAG(FLG_A);
                    break;
                }
            case BYTECODE_ora:
                {
                    u8 with = m->registers[NEXT_BYTE()];
                    LOGICAL_NOT_CMA(|);
                    break;
                }
            case BYTECODE_ora_M:
                {
                    u8 with = memory[FROM_HL()];
                    LOGICAL_NOT_CMA(|);
                    break;
                }
            case BYTECODE_ori:
                {
                    u8 with = NEXT_BYTE();
                    LOGICAL_NOT_CMA(|);
                    break;
                }
            case BYTECODE_xra:
                {
                    u8 with = m->registers[NEXT_BYTE()];
                    LOGICAL_NOT_CMA(^);
                    break;
                }
            case BYTECODE_xra_M:
                {
                    u8 with = m->registers[NEXT_BYTE()];
                    LOGICAL_NOT_CMA(^);
                    break;
                }
            case BYTECODE_xri:
                {
                    u8 with = NEXT_BYTE();
                    LOGICAL_NOT_CMA(^);
                    break;
                }
            case BYTECODE_cma:
                {
                    m->registers[REG_A] = ~m->registers[REG_A];
                    break;
                }
            case BYTECODE_cmc:
                {
                    if(GET_FLAG(FLG_C))
                        RESET_FLAG(FLG_C);
                    else
                        SET_FLAG(FLG_C);
                    break;
                }
            case BYTECODE_cmp:
                {
                    u8 bak = m->registers[REG_A];
                    u8 by = m->registers[NEXT_BYTE()] + 1;
                    SUB();
                    m->registers[REG_A] = bak;
                    break;
                }
            case BYTECODE_cmp_M:
                {
                    u8 bak = m->registers[REG_A];
                    u8 by = memory[FROM_HL()] + 1;
                    SUB();
                    m->registers[REG_A] = bak;
                    break;
                }
            case BYTECODE_cpi:
                {
                    u8 bak = m->registers[REG_A];
                    u8 by = NEXT_BYTE();
                    SUB();
                    m->registers[REG_A] = bak;
                    break;
                }
            case BYTECODE_rlc:
                {
                    u8 d7 = m->registers[REG_A] >> 7;
                    m->registers[REG_FL] |= (d7 << FLG_C); // Set/reset the carry
                    m->registers[REG_A] <<= 1;
                    m->registers[REG_A] |= d7;
                    break;
                }
            case BYTECODE_ral:
                {
                    u8 d7 = m->registers[REG_A] >> 7;
                    u8 d0 = GET_FLAG(FLG_C);
                    m->registers[REG_FL] |= (d7 << FLG_C); // Set/reset the carry
                    m->registers[REG_A] <<= 1;
                    m->registers[REG_A] |= d0;
                    break;
                }
            case BYTECODE_rrc:
                {
                    u8 d0 = m->registers[REG_A] & 1;
                    m->registers[REG_FL] |= (d0 << FLG_C); // Set/reset the carry
                    m->registers[REG_A] >>= 1;
                    m->registers[REG_A] |= (d0 << 7);
                    break;
                }
            case BYTECODE_rar:
                {
                    u8 d0 = m->registers[REG_A] & 1;
                    u8 d7 = GET_FLAG(FLG_C);
                    m->registers[REG_FL] |= (d0 << FLG_C); // Set/reset the carry
                    m->registers[REG_A] >>= 1;
                    m->registers[REG_A] |= (d7 << 7);
                    break;
                }
            case BYTECODE_jmp:
                {
                    JMP_ON(1);
                }
            case BYTECODE_jc:
                {
                    JMP_ON(GET_FLAG(FLG_C));
                }
            case BYTECODE_jnc:
                {
                    JMP_ON(!GET_FLAG(FLG_C));
                }
            case BYTECODE_jz:
                {
                    JMP_ON(GET_FLAG(FLG_Z));
                }
            case BYTECODE_jnz:
                {
                    JMP_ON(!GET_FLAG(FLG_Z));
                }
            case BYTECODE_jp:
                {
                    JMP_ON(!GET_FLAG(FLG_S));
                }
            case BYTECODE_jm:
                {
                    JMP_ON(GET_FLAG(FLG_S));
                }
            case BYTECODE_call:
                {
                    CALL_ON(1);
                }
            case BYTECODE_cc:
                {
                    CALL_ON(GET_FLAG(FLG_C));
                }
            case BYTECODE_cnc:
                {
                    CALL_ON(!GET_FLAG(FLG_C));
                }
            case BYTECODE_cz:
                {
                    CALL_ON(GET_FLAG(FLG_Z));
                }
            case BYTECODE_cnz:
                {
                    CALL_ON(!GET_FLAG(FLG_Z));
                }
            case BYTECODE_cm:
                {
                    CALL_ON(GET_FLAG(FLG_S));
                }
            case BYTECODE_cp:
                {
                    CALL_ON(!GET_FLAG(FLG_S));
                }
            case BYTECODE_ret:
                {
                    RET_ON(1);
                }
            case BYTECODE_rc:
                {
                    RET_ON(GET_FLAG(FLG_C));
                }
            case BYTECODE_rnc:
                {
                    RET_ON(!GET_FLAG(FLG_C));
                }
            case BYTECODE_rz:
                {
                    RET_ON(GET_FLAG(FLG_Z));
                }
            case BYTECODE_rnz:
                {
                    RET_ON(!GET_FLAG(FLG_Z));
                }
            case BYTECODE_rm:
                {
                    RET_ON(GET_FLAG(FLG_S));
                }
            case BYTECODE_rp:
                {
                    RET_ON(!GET_FLAG(FLG_S));
                }
            case BYTECODE_in:
                {
                    fflush(stdin);
                    m->registers[REG_A] = getchar();
                    break;
                }
            case BYTECODE_out:
                {
                    printf("\n0x%x\n", m->registers[REG_A]);
                    fflush(stdout);
                    break;
                }
            case BYTECODE_hlt:
                return;
            case BYTECODE_nop:
                break;
            default:
                break;
        }
        print_machine(m);
    }
}
