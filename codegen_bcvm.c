#include "common.h"

#ifdef USE_BCVM

#include "bytecode.h"
#include "codegen.h"
#include "compiler.h"
#include "display.h"

static Token presentToken;

// Here's a bit of explanation of the approach
// and the architechture of the vm and how it maps
// to the actual 8085 ISA.
//
// The thing is, 8085 had a lot of ambiguous
// opcodes. For example, one 'add' instruction
// can add the accumulator with the content
// at a memory address or with just another
// register. 8085 usually tackled it by denoting
// the memory as either 110 or 111(I can't remember
// correctly now), in the opcode. Which then, the
// decoder would process and do the required thing.
// Since we're not following the internal's of the
// architechture pie to pie, just the ISA part, and
// since we don't have such a compilcated decoder
// (which will just make these type of instructions
// a hell lot complicated with a bunch of `if`s
// hanging out with each of them), we're
// distinguishing the register and the memory
// version at the opcode level, and emitting the
// specific opcode while compilation itself,
// to benefit the runtime.
// So that, we have two 'add' instructions at the vm
// level :
// 1. One that adds two registers, i.e. the usual
// 'add',
// 2. One that adds the accumulator with a memory
// address - 'add_M'.
// Now, we obviously don't want to confuse the
// programmers with the internals of the specific
// architechture of our vm, we just want them
// to remember how 8085 does things. So, while
// compilation, whenever we're seeing a statement
// like 'add m' or 'dad sp', we're replacing the
// instructions with their specific counterparts,
// to make our little vm breathe a little.
// You can see instruction.h, which maps each
// instructions exactly with 8085, and then
// when you see bytecodes.h, which is our version
// of that instruction set, you will see after
// all the instructions from instruction.h,
// some more opcodes are declared
// with suffix _M, or _SP, or _PSW, which
// corresponds to the specific 'add m', or
// 'dad sp' or 'push psw' instructions.
// The compiler just maps each 8085 opcode to
// the specific opcode required for our vm to
// run.

// Get the _M version of opcode
// add m

static Bytecode get_m_version_of(Bytecode code) {
	switch(code) {
		case BYTECODE_mvi: return BYTECODE_mvi_M;
		case BYTECODE_mov: return BYTECODE_mov_M;
		case BYTECODE_dcr: return BYTECODE_dcr_M;
		case BYTECODE_inr: return BYTECODE_inr_M;
		case BYTECODE_add: return BYTECODE_add_M;
		case BYTECODE_sub: return BYTECODE_sub_M;
		case BYTECODE_ana: return BYTECODE_ana_M;
		case BYTECODE_ora: return BYTECODE_ora_M;
		case BYTECODE_xra: return BYTECODE_xra_M;
		case BYTECODE_cmp: return BYTECODE_cmp_M;
		case BYTECODE_adc: return BYTECODE_adc_M;
		case BYTECODE_sbb: return BYTECODE_sbb_M;
		default:
			perr("[Internal Error] M version required for code %d", code);
			token_highlight_source(presentToken);
			return BYTECODE_hlt;
	}
}

// Get the _SP version of the opcode
// dad sp
static Bytecode get_sp_version_of(Bytecode code) {
	switch(code) {
		case BYTECODE_lxi: return BYTECODE_lxi_SP;
		case BYTECODE_inx: return BYTECODE_inx_SP;
		case BYTECODE_dcx: return BYTECODE_dcx_SP;
		case BYTECODE_dad: return BYTECODE_dad_SP;
		default:
			perr("[Internal error] SP version required for code %d", code);
			token_highlight_source(presentToken);
			return BYTECODE_hlt;
	}
}

// Get the _PSW version of the opcode
// push psw
static Bytecode get_psw_version_of(Bytecode code) {
	switch(code) {
		case BYTECODE_pop: return BYTECODE_pop_PSW;
		case BYTECODE_push: return BYTECODE_push_PSW;
		default:
			perr("[Internal Error] PSW version required for code %d", code);
			token_highlight_source(presentToken);
			return BYTECODE_hlt;
	}
}

// Each bytecode mapping to a
// specific type of token, since
// all tokens here are mostly
// instructions except for a few
static Bytecode bytecodes[] = {
    BYTECODE_hlt, // colon
    BYTECODE_hlt, // comma
    BYTECODE_hlt, // identifer
    BYTECODE_hlt, // number

#define INSTRUCTION(name, length) BYTECODE_##name,
#include "instruction.h"
#undef INSTRUCTION

    BYTECODE_hlt, // error
    BYTECODE_hlt  // eof
};

void codegen_no_op(Token t) {
	compiler_write_byte(bytecodes[t.type]);
}

// Compile a register.
// The token must be validated to
// be a register beforehand, which is
// usually done by the parent methods
// which invoke this.
static void codegen_compile_reg(Token t) {
	switch(t.start[0]) {
		case 'a': compiler_write_byte(REG_A); break;
		case 'b': compiler_write_byte(REG_B); break;
		case 'c': compiler_write_byte(REG_C); break;
		case 'd': compiler_write_byte(REG_D); break;
		case 'e': compiler_write_byte(REG_E); break;
		case 'h': compiler_write_byte(REG_H); break;
		case 'l': compiler_write_byte(REG_L); break;
	}
}

// Interface implementation for initial custom bytecode vm
// =======================================================

void codegen_reg(Token t, Token reg) {
	compiler_write_byte(bytecodes[t.type]);
	codegen_compile_reg(reg);
}

void codegen_mem(Token t) {
	compiler_write_byte(get_m_version_of(bytecodes[t.type]));
}

void codegen_regpair(Token t, Token reg) {
	codegen_reg(t, reg);
}

void codegen_sp(Token t) {
	compiler_write_byte(get_sp_version_of(bytecodes[t.type]));
}

void codegen_psw(Token t) {
	compiler_write_byte(get_psw_version_of(bytecodes[t.type]));
}

void codegen_mov_r_r(Token dest, Token source) {
	compiler_write_byte(BYTECODE_mov);
	codegen_compile_reg(dest);
	codegen_compile_reg(source);
}

void codegen_mov_r_m(Token dest) {
	compiler_write_byte(BYTECODE_mov_R);
	codegen_compile_reg(dest);
}

void codegen_mov_m_r(Token source) {
	compiler_write_byte(BYTECODE_mov_M);
	codegen_compile_reg(source);
}

#endif
