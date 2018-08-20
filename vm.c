#include "vm.h"

#ifdef USE_BCVM

#include <stdio.h>

#include "bytecode.h"
#include "display.h"

#define GET_FLAG(x)             ((m->registers[REG_FL] >> x) & 1)

void run(Machine *m, u8 *memory, u8 step){
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
                - (temp >> 4) - (temp >> 5) - (temp >> 6) - (temp >> 7);    \
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
        if(cond)                    \
            m->pc = addr;           \
        break;

    #define CALL_ON(cond)                               \
        u16 addr = NEXT_DWORD();                        \
        if(cond){                                       \
            memory[m->sp - 1] = (m->pc & 0xff00) >> 8;  \
            memory[m->sp - 2] = m->pc & 0x00ff;         \
            m->sp -= 2;                                 \
            m->pc = addr;                               \
        }                                               \
        break;

    #define RET_ON(cond)                        \
        if(cond){                               \
            m->pc = memory[m->sp];              \
            m->pc |= (memory[m->sp + 1] << 8);  \
            m->sp += 2;                         \
        }                                       \
        break;

    #define DAD()                                   \
        u32 res = FROM_PAIR(REG_H, REG_L) + with;   \
        if(res > 0xffff)                            \
            SET_FLAG(FLG_C);                        \
        m->registers[REG_H] = (res & 0xff00) >> 8;  \
        m->registers[REG_L] = res & 0x00ff;
    
    Bytecode opcode;
    while((opcode = (Bytecode)NEXT_BYTE()) != BYTECODE_hlt){
        //printf("\n");
        //bytecode_disassemble_in_context(memory, m->pc - 1, m);
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
            case BYTECODE_lhld:
                {
                    u16 addr = NEXT_DWORD();
                    m->registers[REG_L] = memory[addr];
                    m->registers[REG_H] = memory[addr + 1];
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
                    INIT_FLG_P(res);
                    INIT_FLG_A(m->registers[reg], 1);
                    m->registers[reg] = res & 0xff;
                    break;
                }
            case BYTECODE_inr_M:
                {
                    u16 res = memory[FROM_HL()] + 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    INIT_FLG_P(res);
                    INIT_FLG_A(memory[FROM_HL()], 1);
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
            case BYTECODE_sbb:
                {
                    u8 by = m->registers[NEXT_BYTE()] + GET_FLAG(FLG_C);
                    SUB();
                    break;
                }
            case BYTECODE_sbb_M:
                {
                    u8 by = memory[FROM_HL()] + GET_FLAG(FLG_C);
                    SUB();
                    break;
                }
            case BYTECODE_sbi:
                {
                    u8 by = NEXT_BYTE() + GET_FLAG(FLG_C);
                    SUB();
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
            case BYTECODE_dad:
                {
                    u8 reg = NEXT_BYTE();
                    u16 with = FROM_PAIR(reg, reg+1);
                    DAD();
                    break;
                }
            case BYTECODE_dad_SP:
                {
                    u16 with = m->sp;
                    DAD();
                    break;
                }
            case BYTECODE_dcr:
                {
                    u8 reg = NEXT_BYTE();
                    u16 res = m->registers[reg] - 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    INIT_FLG_P(res);
                    INIT_FLG_A(m->registers[reg], -1);
                    m->registers[reg] = res & 0xff;
                    break;
                }
            case BYTECODE_dcr_M:
                {
                    u16 res = memory[FROM_HL()] - 1;
                    INIT_FLG_S(res);
                    INIT_FLG_Z(res);
                    INIT_FLG_P(res);
                    INIT_FLG_A(memory[FROM_HL()], -1);
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
                    u8 with = memory[FROM_HL()];
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
                    CHANGE_FLAG(FLG_C, !(GET_FLAG(FLG_C)));
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
                    CHANGE_FLAG(FLG_C, d7); // Set/reset the carry
                    m->registers[REG_A] <<= 1;
                    m->registers[REG_A] |= d7;
                    break;
                }
            case BYTECODE_ral:
                {
                    u8 d7 = m->registers[REG_A] >> 7;
                    u8 d0 = GET_FLAG(FLG_C);
                    CHANGE_FLAG(FLG_C, d7); // Set/reset the carry
                    m->registers[REG_A] <<= 1;
                    m->registers[REG_A] |= d0;
                    break;
                }
            case BYTECODE_rrc:
                {
                    u8 d0 = m->registers[REG_A] & 1;
                    CHANGE_FLAG(FLG_C, d0); // Set/reset the carry
                    m->registers[REG_A] >>= 1;
                    m->registers[REG_A] |= (d0 << 7);
                    break;
                }
            case BYTECODE_rar:
                {
                    u8 d0 = m->registers[REG_A] & 1;
                    u8 d7 = GET_FLAG(FLG_C);
                    CHANGE_FLAG(FLG_C, d0); // Set/reset the carry
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
            case BYTECODE_jpe:
                {
                    JMP_ON(GET_FLAG(FLG_P));
                }
            case BYTECODE_jpo:
                {
                    JMP_ON(!GET_FLAG(FLG_P));
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
            case BYTECODE_cpe:
                {
                    CALL_ON(GET_FLAG(FLG_P));
                }
            case BYTECODE_cpo:
                {
                    CALL_ON(!GET_FLAG(FLG_P));
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
            case BYTECODE_rpe:
                {
                    RET_ON(GET_FLAG(FLG_P));
                }
            case BYTECODE_rpo:
                {
                    RET_ON(!GET_FLAG(FLG_P));
                }
            case BYTECODE_in:
                {
                    fflush(stdin);
                    u32 in;
                    printf("\n[in:0x%x] ", NEXT_BYTE());
                    scanf("%x", &in);
                    m->registers[REG_A] = (u8)in;
                    break;
                }
            case BYTECODE_out:
                {
                    printf("\n[out:0x%x] 0x%x\n", NEXT_BYTE(), m->registers[REG_A]);
                    fflush(stdout);
                    break;
                }
            case BYTECODE_pchl:
                {
                    m->pc = m->registers[REG_L];
                    m->pc |= (m->registers[REG_H] << 8);
                    break;
                }
            case BYTECODE_pop:
                {
                    u8 reg = NEXT_BYTE();
                    m->registers[reg + 1] = memory[m->sp];
                    m->sp++;
                    m->registers[reg] = memory[m->sp];
                    m->sp++;
                    break;
                }
            case BYTECODE_pop_PSW:
                {
                    m->registers[REG_FL] = memory[m->sp];
                    m->sp++;
                    m->registers[REG_A] = memory[m->sp];
                    m->sp++;
                    break;
                }
            case BYTECODE_push:
                {
                    u8 reg = NEXT_BYTE();
                    memory[m->sp - 1] = m->registers[reg];
                    memory[m->sp - 2] = m->registers[reg + 1];
                    m->sp -= 2;
                    break;
                }
            case BYTECODE_push_PSW:
                {
                    memory[m->sp - 1] = m->registers[REG_A];
                    memory[m->sp - 2] = m->registers[REG_FL];
                    m->sp -= 2;
                    break;
                }
            case BYTECODE_shld:
                {
                    u16 addr = NEXT_DWORD();
                    memory[addr] = m->registers[REG_L];
                    memory[addr + 1] = m->registers[REG_H];
                    break;
                }
            case BYTECODE_sphl:
                {
                    m->sp = FROM_HL();
                    break;
                }
            case BYTECODE_stc:
                {
                    SET_FLAG(FLG_C);
                    break;
                }
            case BYTECODE_xchg:
                {
                    u8 td = m->registers[REG_D];
                    u8 te = m->registers[REG_E];
                    m->registers[REG_D] = m->registers[REG_H];
                    m->registers[REG_E] = m->registers[REG_L];
                    m->registers[REG_H] = td;
                    m->registers[REG_L] = te;
                    break;
                }
            case BYTECODE_xthl:
                {
                    u8 td = memory[m->sp + 1];
                    u8 te = memory[m->sp];
                    memory[m->sp + 1] = m->registers[REG_H];
                    memory[m->sp] = m->registers[REG_L];
                    m->registers[REG_H] = td;
                    m->registers[REG_L] = te;
                    break;
                }
            case BYTECODE_hlt:
                m->isbroken = 0;
                return;
            case BYTECODE_nop:
                break;
            default:
                break;
        }
        if(machine_on_breakpoint(m, memory, step))
            return;
        //print_machine(m);
    }
    m->isbroken = 0;
}

#endif
