#include "bytecode.h"
#include "display.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// nibble3  nibble2     nibble1     nibble0
static uint8_t extract_nibble(uint16_t ins, uint8_t pos) {
	return (uint8_t)(ins >> (pos * 4) & 0x0f);
}

// clang-format off

/*
 *  types : ['16-bit', '16-bita', '8-bit', 'None', 'R,8-bit/M,8-bit', 'R,R/R,M/M,R', 'R/M', 'Rx', 'Rx,16-bit', 'RxBD']
 *              0           1       2        3          4                   5           6    7          8        9 
 *              Excess '/' elements will be stored in indices (9 + orig), 'M,R' will be the last element
 */

// clang-format on

static uint8_t type_isdual(uint8_t type) {
	return type == 4 || type == 5 || type == 6;
}

static uint8_t type_isthrice(uint8_t type) {
	return type == 5;
}

static const char *type_strings[] = {
    "16-bit data",
    "16-bit address",
    "8-bit data",
    "None",
    "Register"
    ", "
    "8-bit data",
    "Register"
    ", "
    "Register",
    "Register",
    "Register pair",
    "Register pair"
    ", "
    "16-bit data",
    "Register pair (B/D)",
    "Memory"
    ", "
    "8-bit data", // 4
    "Register"
    ", "
    "Memory", // 5
    "Memory", // 6
    "Memory"
    ", "
    "Register" // 5.1
};

static const char *type_get_str(uint16_t code, uint8_t part) {
	uint8_t typ = extract_nibble(code, 3);
	if(part == 1)
		return type_strings[typ];
	if(part == 3) {
		if(type_isthrice(typ))
			return type_strings[13];
		return " ";
	}
	if(!type_isdual(typ))
		return " ";
	return type_strings[10 + typ - 4];
}

static char example[20] = {0}, partins[6] = {0}, operand1[7] = {0},
            operand2[7] = {0};

static uint16_t get_random_16() {
	// srand(time(NULL));
	return random_at_most(0xffff - 1);
}

static uint16_t get_random_8() {
	// srand(time(NULL));
	return random_at_most(0xff - 1);
}

static char get_random_reg() {
	// srand(time(NULL));
	i64 ret = random_at_most(6);
	return ret < 5 ? ret + 'a' : ret == 5 ? 'h' : 'l';
}

static const char *get_random_pair() {
	// srand(time(NULL));
	uint8_t res = random_at_most(4);
	switch(res) {
		case 0: return "b";
		case 1: return "d";
		case 2: return "h";
		case 3: return "sp";
		case 4: return "psw";
	}
	return "";
}

static void type_gen_example(uint16_t code, uint8_t part, int ins) {
	uint8_t typ = extract_nibble(code, 3);
	sprintf(partins, "%s", " ");
	sprintf(operand1, "%s", " ");
	sprintf(operand2, "%s", " ");
	if(part == 1) {
		sprintf(partins, "%s ", bytecode_get_string((Bytecode)ins));
		switch(typ) {
			case 0:
			case 1: {
				uint16_t el = get_random_16();
				sprintf(operand1, "%xh", el);
				break;
			}
			case 2: {
				uint16_t el = get_random_8();
				sprintf(operand1, "%xh", el);
				break;
			}
			case 3: {
				break;
			}
			case 4: {
				sprintf(operand1, "%c,", get_random_reg());
				sprintf(operand2, " %xh", get_random_8());
				break;
			}
			case 5: {
				sprintf(operand1, "%c,", get_random_reg());
				sprintf(operand2, " %c", get_random_reg());
				break;
			}
			case 6: {
				sprintf(operand1, "%c", get_random_reg());
				break;
			}
			case 7: {
				sprintf(operand1, "%s", get_random_pair());
				break;
			}
			case 8: {
				sprintf(operand1, "%s,", get_random_pair());
				sprintf(operand2, " %xh", get_random_16());
				break;
			}
			case 9: {
				sprintf(operand1, "%c", get_random_8() & 1 ? 'b' : 'd');
				break;
			}
		}
	} else if(part == 2 && type_isdual(typ)) {
		sprintf(partins, "%s ", bytecode_get_string((Bytecode)ins));
		switch(typ) {
			case 4:
				sprintf(operand1, "m,");
				sprintf(operand2, " %xh", get_random_8());
				break;
			case 5:
				sprintf(operand1, "%c,", get_random_reg());
				sprintf(operand2, " m");
				break;
			case 6: sprintf(operand1, "m"); break;
		}
	} else if(part == 3) {
		sprintf(partins, "%s ", bytecode_get_string((Bytecode)ins));
		sprintf(operand1, "m,");
		sprintf(operand2, " %c", get_random_reg());
	}
	sprintf(example, "%s%s%s", partins, operand1, operand2);
}

/*
 *  lengths : ['1', '1/1', '1/1/1', '2', '2/2', '3']
 *              0    1       2       3      4    5
 */

static const char *len_string[] = {"1 byte",  "1 byte",  "1 byte",
                                   "2 bytes", "2 bytes", "3 bytes"};

static const char *len_get_str(uint16_t code) {
	return len_string[extract_nibble(code, 2)];
}

/*
 *  m/cs : ['1', '1/2', '1/2/2', '1/3', '2', '2/3', '2/5', '3', '4', '5']
 *           0      1       2       3    4    5      6      7    8    9
 */

static uint8_t mc_istwice(uint8_t mc) {
	return mc == 1 || mc == 2 || mc == 3 || mc == 5 || mc == 7;
}

static uint8_t mc_get(uint16_t code, uint8_t part) {
	uint8_t mct = extract_nibble(code, 1);
	switch(part) {
		case 1: {
			switch(mct) {
				case 0:
				case 1:
				case 2:
				case 3: return 1;
				case 4:
				case 5:
				case 6: return 2;
				case 7: return 3;
				case 8: return 4;
				case 9: return 5;
			}
		} break;
		case 2: {
			switch(mct) {
				case 1:
				case 2: return 2;
				case 3:
				case 5: return 3;
				case 6: return 5;
			}
		} break;
	}

	return 2;
}

// clang-format off

/*
 *  t/ss : ['10', '12', '13', '16', '18', '4', '4/10', '4/7', '4/7/7', '5', '6', '6/12', '7', '7/10', '9/18']
 *           0      1    2      3    4     5     6      7       8       9    10     11    12    13      14
 */

// clang-format on

static uint8_t tstate_istwice(uint8_t t) {
	return t == 6 || t == 7 || t == 8 || t == 11 || t == 13 || t == 14;
}

static uint8_t tstate_get(uint16_t code, uint8_t part) {
	uint8_t ts = extract_nibble(code, 0);
	switch(part) {
		case 1:
			switch(ts) {
				case 0: return 10;
				case 1: return 12;
				case 2: return 13;
				case 3: return 16;
				case 4: return 18;
				case 5:
				case 6:
				case 7:
				case 8: return 4;
				case 9: return 5;
				case 10:
				case 11: return 6;
				case 12:
				case 13: return 7;
				case 14: return 9;
			}
			break;
		case 2:
			switch(ts) {
				case 6: return 10;
				case 7:
				case 8: return 7;
				case 11: return 12;
				case 13: return 10;
				case 14: return 18;
			}
			break;
	}
	return 7;
}

static uint16_t opcode_details[] = {
    0x234c, // ACI
    0x6117, // ADC
    0x6117, // ADD
    0x234c, // ADI
    0x6117, // ANA
    0x234c, // ANI
    0x1594, // CALL
    0x156e, // CC
    0x156e, // CM
    0x3005, // CMA
    0x3005, // CMC
    0x6117, // CMP
    0x156e, // CNC
    0x156e, // CNZ
    0x156e, // CP
    0x156e, // CPE
    0x234c, // CPI
    0x156e, // CPO
    0x156e, // CZ
    0x3005, // DAA
    0x7070, // DAD
    0x6136, // DCR
    0x700a, // DCX
    // 0x3005,   // DI
    // 0x3005,   // EI
    0x3049, // HLT
    0x2370, // IN
    0x6136, // INR
    0x700a, // INX
    0x055d, // JC
    0x055d, // JM
    0x0570, // JMP
    0x055d, // JNC
    0x055d, // JNZ
    0x055d, // JP
    0x055d, // JPE
    0x055d, // JPO
    0x055d, // JZ
    0x1582, // LDA
    0x904c, // LDAX
    0x1593, // LHLD
    0x8570, // LXI
    0x5228, // MOV
    0x445d, // MVI
    0x3005, // NOP
    0x6117, // ORA
    0x234c, // ORI
    0x2370, // OUT
    0x300a, // PCHL
    0x7070, // POP
    0x7071, // PUSH
    0x3005, // RAL
    0x3005, // RAR
    0x303b, // RC
    0x3070, // RET
    // 0x3005,   // RIM
    0x3005, // RLC
    0x303b, // RM
    0x303b, // RNC
    0x303b, // RNZ
    0x303b, // RP
    0x303b, // RPE
    0x303b, // RPO
    0x3005, // RRC
    0x303b, // RZ
    0x6117, // SBB
    0x234c, // SBI
    0x1593, // SHLD
    // 0x3005,   // SIM
    0x300a, // SPHL
    0x0582, // STA
    0x904c, // STAX
    0x3005, // STC
    0x6117, // SUB
    0x234c, // SUI
    0x3005, // XCHG
    0x6117, // XRA
    0x234c, // XRI
    0x3093, // XTHL
};

void instruction_print_details(int num) {
	uint16_t op     = opcode_details[num];
	int      target = 2;
	target += type_isdual(extract_nibble(op, 3)) ||
	          mc_istwice(extract_nibble(op, 1)) ||
	          tstate_istwice(extract_nibble(op, 0));
	target += type_isthrice(extract_nibble(op, 3));
	// Headers
	printf("\n%9sOperand Type%*.sLength%2s\tM/C\tT/S\t Example\n", " ", 11, " ",
	       " ");
	int totallength = 30 + 4 + 10 + 4 + 2 + 4 + 2 + 4 + 7 + 4 + 4;
	for(int i = 0; i < totallength; i++) printf("-");
	for(int i = 1; i < target; i++) {
		// Print the operand type
		const char *str   = type_get_str(op, i);
		siz         len   = strlen(str);
		int         extra = (30 - len) / 2;
		printf("\n%*.s%s%*.s\t", extra, " ", str, extra, " ");
		// Print the instruction length (only for the first time)
		printf("%-10s\t",
		       i == 1 ? len_get_str(op) : (str[0] != ' ' ? " " : "(if taken)"));
		// Print the machine cycle
		printf("%2d\t", mc_get(op, i));
		// Print the t-states
		printf("%2d\t", tstate_get(op, i));
		// Print an example
		type_gen_example(op, i, num);
		printf("%s\n", example);
	}
	printf("\n\n");
}
