#include "bytecode.h"
#include "display.h"
#include "vm.h"

#define GET_FLAG(x) ((m->registers[REG_FL] >> x) & 1)

void machine_print(Machine *m) {
	char regs[] = {'A', 'B', 'C', 'D', 'E', 'H', 'L'};
	pgrn("\n[Registers]\t");
	for(u8 i = 0; i < 7; i++) pcyn("%4c\t", regs[i], " ");

	pmgn("\n          \t");

	phmgn("", "0x%02x\t", m->registers[REG_A]);
	phmgn("", "0x%02x\t", m->registers[REG_B]);
	phmgn("", "0x%02x\t", m->registers[REG_C]);
	phmgn("", "0x%02x\t", m->registers[REG_D]);
	phmgn("", "0x%02x\t", m->registers[REG_E]);
	phmgn("", "0x%02x\t", m->registers[REG_H]);
	phmgn("", "0x%02x\t", m->registers[REG_L]);

	pgrn("\n[FLAGS] ");
	char flags[] = {'S', 'Z', ' ', 'A', ' ', 'P', ' ', 'C'};
	for(u8 i = 0; i < 8; i++) pcyn("%c ", flags[i]);
	pgrn("\n        ");
	for(int i = 7; i >= 0; i--) phred("", "%d ", GET_FLAG(i));

	phblue("\n[PC] ", "0x%0x", m->pc);
	phylw("\n[SP] ", "0x%0x", m->sp);
}

bool machine_add_breakpoint(Machine *m, u16 addr) {
	if(m->breakpoint_pointer == MAX_BREAKPOINT_COUNT)
		return false;
	for(u16 i = 0; i < m->breakpoint_pointer; i++) {
		if(m->breakpoints[i] == addr)
			return true;
	}
	m->breakpoints[m->breakpoint_pointer++] = addr;
	return true;
}

bool machine_on_breakpoint(Machine *m, u8 *memory, u8 step) {
	if(step) {
		m->isbroken = 1;
		phgrn("\n[step]", " Stepped on address 0x%x", m->pc);
		machine_print(m);
		bytecode_disassemble(memory, m->pc);
		return true;
	}
	for(u16 i = 0; i < m->breakpoint_pointer; i++) {
		if(m->breakpoints[i] == m->pc) {
			m->isbroken = 1;
			phgrn("\n[break]", " Breakpoint caught on address 0x%x",
			      m->breakpoints[i]);
			machine_print(m);
			bytecode_disassemble(memory, m->breakpoints[i]);
			return true;
		}
	}
	return false;
}

bool machine_remove_breakpoint(Machine *m, u16 addr) {
	for(u16 i = 0; i < m->breakpoint_pointer; i++) {
		if(m->breakpoints[i] == addr) {
			while(i < m->breakpoint_pointer - 1) {
				m->breakpoints[i] = m->breakpoints[i + 1];
				i++;
			}
			m->breakpoint_pointer--;
			return true;
		}
	}
	return false;
}

void machine_reset_breakpoints(Machine *m) {
	m->breakpoint_pointer = 0;
}
