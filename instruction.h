// This list MUST be alphabetically sorted
// INSTRUCTION(instruction_opcode, instruction_string_size)
// If an instruction has a _M or _R version, define them
// at the end of Bytecode enum, and create a replacement
// case in compiler.c
INSTRUCTION(aci, 3)
INSTRUCTION(adc, 3)
INSTRUCTION(add, 3)
INSTRUCTION(adi, 3)
INSTRUCTION(ana, 3)
INSTRUCTION(ani, 3)
INSTRUCTION(call, 4)
INSTRUCTION(cc, 2)
INSTRUCTION(cm, 2)
INSTRUCTION(cma, 3)
INSTRUCTION(cmc, 3)
INSTRUCTION(cmp, 3)
INSTRUCTION(cnc, 3)
INSTRUCTION(cnz, 3)
INSTRUCTION(cp, 2)
INSTRUCTION(cpi, 3)
INSTRUCTION(cz, 2)
INSTRUCTION(daa, 3)
INSTRUCTION(dad, 3)
INSTRUCTION(dcr, 3)
INSTRUCTION(dcx, 3)
INSTRUCTION(hlt, 3)
INSTRUCTION(in, 2)
INSTRUCTION(inr, 3)
INSTRUCTION(inx, 3)
INSTRUCTION(jc, 2)
INSTRUCTION(jm, 2)
INSTRUCTION(jmp, 3)
INSTRUCTION(jnc, 3)
INSTRUCTION(jnz, 3)
INSTRUCTION(jp, 2)
INSTRUCTION(jz, 2)
INSTRUCTION(lda, 3)
INSTRUCTION(ldax, 4)
INSTRUCTION(lhld, 4)
INSTRUCTION(lxi, 3)
INSTRUCTION(mov, 3)
INSTRUCTION(mvi, 3)
INSTRUCTION(nop, 3)
INSTRUCTION(ora, 3)
INSTRUCTION(ori, 3)
INSTRUCTION(out, 3)
INSTRUCTION(pchl, 4)
INSTRUCTION(pop, 3)
INSTRUCTION(push, 4)
INSTRUCTION(ral, 3)
INSTRUCTION(rar, 3)
INSTRUCTION(rc, 2)
INSTRUCTION(ret, 3)
INSTRUCTION(rlc, 3)
INSTRUCTION(rm, 2)
INSTRUCTION(rnc, 3)
INSTRUCTION(rnz, 3)
INSTRUCTION(rp, 2)
INSTRUCTION(rrc, 3)
INSTRUCTION(rz, 2)
INSTRUCTION(sbb, 3)
INSTRUCTION(sbi, 3)
INSTRUCTION(shld, 4)
INSTRUCTION(sphl, 4)
INSTRUCTION(sta, 3)
INSTRUCTION(stax, 4)
INSTRUCTION(stc, 3)
INSTRUCTION(sub, 3)
INSTRUCTION(sui, 3)
INSTRUCTION(xra, 3)
INSTRUCTION(xri, 3)
