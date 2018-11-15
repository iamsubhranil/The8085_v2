#include "common.h"
#include "codegen.h"
#include "compiler.h"
#include "display.h"

// Get a register number in 8085 convention
static u8 codegen_get_reg_number(Token t) {
	switch(t.start[0]) {
		case 'a': return 7;
		case 'b': return 0;
		case 'c': return 1;
		case 'd': return 2;
		case 'e': return 3;
		case 'h': return 4;
		case 'l': return 5;
	}
	perr("[Internal error] Register number required for '%.*s'", t.length,
	     t.start);
	return 0;
}

static u8 codegen_get_opcode(Token t) {
	switch(t.type) {
		case TOKEN_aci: return 0xCE;
		case TOKEN_adi: return 0xC6;
		case TOKEN_ani: return 0xE6;
		case TOKEN_call: return 0xCD;
		case TOKEN_cc: return 0xDC;
		case TOKEN_cm: return 0xFC;
		case TOKEN_cma: return 0x2F;
		case TOKEN_cmc: return 0x3F;
		case TOKEN_cnc: return 0xD4;
		case TOKEN_cnz: return 0xC4;
		case TOKEN_cp: return 0xF4;
		case TOKEN_cpe: return 0xEC;
		case TOKEN_cpi: return 0xFE;
		case TOKEN_cpo: return 0xE4;
		case TOKEN_cz: return 0xCC;
		case TOKEN_daa: return 0x27;
		case TOKEN_hlt: return 0x76;
		case TOKEN_in: return 0xDB;
		case TOKEN_jc: return 0xDA;
		case TOKEN_jm: return 0xFA;
		case TOKEN_jmp: return 0xC3;
		case TOKEN_jnc: return 0xD2;
		case TOKEN_jnz: return 0xC2;
		case TOKEN_jp: return 0xF2;
		case TOKEN_jpe: return 0xEA;
		case TOKEN_jpo: return 0xE2;
		case TOKEN_jz: return 0xCA;
		case TOKEN_lda: return 0x3A;
		case TOKEN_lhld: return 0x2A;
		case TOKEN_nop: return 0x00;
		case TOKEN_ori: return 0xF6;
		case TOKEN_out: return 0xD3;
		case TOKEN_pchl: return 0xE9;
		case TOKEN_ral: return 0x17;
		case TOKEN_rar: return 0x1F;
		case TOKEN_rc: return 0xD8;
		case TOKEN_ret: return 0xC9;
		case TOKEN_rlc: return 0x07;
		case TOKEN_rm: return 0xf8;
		case TOKEN_rnc: return 0xD0;
		case TOKEN_rnz: return 0xC0;
		case TOKEN_rp: return 0xF0;
		case TOKEN_rpe: return 0xE8;
		case TOKEN_rpo: return 0xE0;
		case TOKEN_rrc: return 0x0F;
		case TOKEN_rz: return 0xC8;
		case TOKEN_sbi: return 0xDE;
		case TOKEN_shld: return 0x22;
		case TOKEN_sphl: return 0xF9;
		case TOKEN_sta: return 0x32;
		case TOKEN_stc: return 0x37;
		case TOKEN_sui: return 0xD6;
		case TOKEN_xchg: return 0xEB;
		case TOKEN_xri: return 0xEE;
		case TOKEN_xthl: return 0xE3;
		default: break;
	}
	perr("[Internal error] Opcode required for token '%.*s'!", t.length,
	     t.start);
	return 0x00;
}

// Encode an instruction with one register
// or memory operand
static void codegen_write_reg_or_mem(Token t, u8 num) {
	switch(t.type) {
		case TOKEN_adc: compiler_write_byte(0x88 | num); break;
		case TOKEN_add: compiler_write_byte(0x80 | num); break;
		case TOKEN_ana: compiler_write_byte(0xA0 | num); break;
		case TOKEN_cmp: compiler_write_byte(0xB8 | num); break;
		case TOKEN_dcr: compiler_write_byte(0x05 | (num << 3)); break;
		case TOKEN_inr: compiler_write_byte(0x04 | (num << 3)); break;
		case TOKEN_mvi: compiler_write_byte(0x06 | (num << 3)); break;
		case TOKEN_ora: compiler_write_byte(0xB0 | num); break;
		case TOKEN_sbb: compiler_write_byte(0x98 | num); break;
		case TOKEN_sub: compiler_write_byte(0x90 | num); break;
		case TOKEN_xra: compiler_write_byte(0xA8 | num); break;
		default:
			perr("[Internal error] Register or memory required for token "
			     "'%.*s'!",
			     t.length, t.start);
			break;
	}
}

// Encode an instruction with one register
// pair or sp/psw operand
static void codegen_write_regpair(Token t, u8 reg) {
	// dad, dcx, inx, lxi, pop, push
	switch(t.type) {
		case TOKEN_dad: compiler_write_byte(0x09 | (reg << 4)); break;
		case TOKEN_dcx: compiler_write_byte(0x0B | (reg << 4)); break;
		case TOKEN_inx: compiler_write_byte(0x03 | (reg << 4)); break;
		case TOKEN_ldax: compiler_write_byte(0x0A | (reg << 4)); break;
		case TOKEN_lxi: compiler_write_byte(0x01 | (reg << 4)); break;
		case TOKEN_pop: compiler_write_byte(0xC1 | (reg << 4)); break;
		case TOKEN_push: compiler_write_byte(0xC5 | (reg << 4)); break;
		case TOKEN_stax: compiler_write_byte(0x02 | (reg << 4)); break;
		default:
			perr("[Internal error] Regpair required for token '%.*s'!",
			     t.length, t.start);
			break;
	}
}

void codegen_no_op(Token t) {
	compiler_write_byte(codegen_get_opcode(t));
}

// Interface implementation for original 8085 bytecode
// ===================================================

void codegen_reg(Token t, Token reg) {
	codegen_write_reg_or_mem(t, codegen_get_reg_number(reg));
}

void codegen_mem(Token t) {
	codegen_write_reg_or_mem(t, 6);
}

void codegen_regpair(Token t, Token rp) {
	codegen_write_regpair(t, codegen_get_reg_number(rp) / 2);
}

void codegen_sp(Token t) {
	codegen_write_regpair(t, 3);
}

void codegen_psw(Token t) {
	codegen_write_regpair(t, 3);
}

void codegen_mov_r_r(Token dest, Token source) {
	u8 d = codegen_get_reg_number(dest);
	u8 s = codegen_get_reg_number(source);
	compiler_write_byte(0x40 | (d << 3) | s);
}

void codegen_mov_r_m(Token dest) {
	u8 d = codegen_get_reg_number(dest);
	compiler_write_byte(0x40 | (d << 3) | 6);
}

void codegen_mov_m_r(Token source) {
	compiler_write_byte(0x40 | (6 << 3) | codegen_get_reg_number(source));
}
