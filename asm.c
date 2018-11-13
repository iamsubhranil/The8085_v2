#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm.h"
#include "compiler.h"
#include "cosmetic.h"
#include "display.h"
#include "util.h"

static u8 *        memory         = NULL;
static u16         pointer        = 0;
static char        prefix[30]     = {0}; //  [c050] -->
static const char *descriptions[] = {
    "Add immediate to accumulator with carry",                  // ACI
    "Add register or memory to accumulator with carry",         // ADC
    "Add register or memory to accumulator",                    // ADD
    "Add immediate to accumulator",                             // ADI
    "Logical AND register or memory with accumulator",          // ANA
    "Logical AND immediate with accumulator",                   // ANI
    "Call a subroutine unconditionally",                        // CALL
    "Call on carry (C = 1)",                                    // CC
    "Call on minus (S = 1)",                                    // CM
    "Complement accumulator",                                   // CMA
    "Complement carry",                                         // CMC
    "Compare with accumulator",                                 // CMP
    "Call on no carry (C = 0)",                                 // CNC
    "Call on no zero (Z = 0)",                                  // CNZ
    "Call on positive (S = 0)",                                 // CP
    "Call on parity even (P = 1)",                              // CPE
    "Compare immediate with accumulator",                       // CPI
    "Call on parity odd (P = 0)",                               // CPO
    "Call on zero (Z = 1)",                                     // CZ
    "Decimal adjust accumulator",                               // DAA
    "Add register pair to H and L registers",                   // DAD
    "Decrement content of register or memory by 1",             // DCR
    "Decrement content of register pair by 1",                  // DCX
    "Halt and enter wait state",                                // HLT
    "Input data to accumulator from the specified port",        // IN
    "Increment content of register or memory by 1",             // INR
    "Increment content of register pair by 1",                  // INX
    "Jump on carry (C = 1)",                                    // JC
    "Jump on minus (S = 1)",                                    // JM
    "Jump unconditionally",                                     // JMP
    "Jump on no carry (C = 0)",                                 // JNC
    "Jump on no zero (Z = 0)",                                  // JNZ
    "Jump on positive (S = 0)",                                 // JP
    "Jump on parity even (P = 1)",                              // JPE
    "Jump on parity odd (P = 0)",                               // JPO
    "Jump on zero (Z = 1)",                                     // JZ
    "Load accumulator direct",                                  // LDA
    "Load accumulator indirect",                                // LDAX
    "Load H and L registers direct",                            // LHLD
    "Load register pair immediate",                             // LXI
    "Move (copy) from source to destination",                   // MOV
    "Move immediate",                                           // MVI
    "No operation",                                             // NOP
    "Logical OR register or memory with accumulator",           // ORA
    "Logical OR immediate with accumulator",                    // ORI
    "Output data from accumulator to the specified port",       // OUT
    "Load program counter with HL contents",                    // PCHL
    "Pop off top of the stack to a register pair",              // POP
    "Push a register pair to top of the stack",                 // PUSH
    "Rotate accumulator left through carry",                    // RAL
    "Rotate accumulator right through carry",                   // RAR
    "Return on carry (C = 1)",                                  // RC
    "Return unconditionally",                                   // RET
    "Rotate accumulator left",                                  // RLC
    "Return on minus (S = 1)",                                  // RM
    "Return on no carry (C = 0)",                               // RNC
    "Return on no zero (Z = 0)",                                // RNZ
    "Return on positive (S = 0)",                               // RP
    "Return on parity even (P = 1)",                            // RPE
    "Return on parity odd (P = 0)",                             // RPO
    "Rotate accumulator right",                                 // RRC
    "Return on zero (Z = 1)",                                   // RZ
    "Subtract register or memory from accumulator with borrow", // SBB
    "Subtract immediate from accumulator with borrow",          // SBI
    "Store H and L registers direct",                           // SHLD
    "Copy H and L registers to the stack pointer",              // SPHL
    "Store accumulator direct",                                 // STA
    "Store accumulator indirect",                               // STAX
    "Set carry",                                                // STC
    "Subtract register or memory from accumulator",             // SUB
    "Subtract immediate from accumulator",                      // SUI
    "Exchange H and L with D and E",                            // XCHG
    "Exclusive OR register or memory with accumulator",         // XRA
    "Exclusive OR immediate with accumulator",                  // XRI
    "Exchange H and L with top of the stack",                   // XTHL
};

static void update_prefix() {
	sprintf(prefix, ANSI_FONT_BOLD "[0x%04x] >>" ANSI_COLOR_RESET, pointer);
}

static void parse_action(CellStringParts csp, Cell *cell) {
	char *str  = NULL;
	siz   size = 0, bak = 0;
	for(siz i = 0; i < csp.part_count; i++) {
		bak  = size;
		size = strlen(csp.parts[i]) + 1;
		str  = (char *)realloc(str, bak + size);
		memcpy(&str[bak], csp.parts[i], size);
		size += bak;
		str[size - 1] = ' ';
	}
	str                    = (char *)realloc(str, size + 1);
	str[size]              = 0;
	u16               pbak = pointer;
	CompilationStatus res  = compile(str, memory, 0xffff, &pointer);
	switch(res) {
		case PARSE_ERROR:
		case LABEL_FULL:
		case MEMORY_FULL:
		case EMPTY_PROGRAM:
			perr("Compilation aborted!");
			pointer = pbak;
			break;
		case LABELS_PENDING:
			pinfo("To avoid erroneous results, either declare or manually "
			      "patch the label before execution.");
		case NO_HLT:
		case COMPILE_OK:
			update_prefix();
			free(cell->prefix);
			cell->prefix = strdup(prefix);
			for(u16 i = pbak; i < pointer; i++) {
				pred("\n0x%04x: ", i);
				printf("0x%02x", memory[i]);
			}
			break;
	}
	free(str);
}

static void label_action(CellStringParts csp, Cell *c) {
	char *bak = csp.parts[0];
	for(siz i = 0; i < csp.part_count - 1; i++) {
		csp.parts[i] = csp.parts[i + 1];
	}
	csp.part_count--;
	parse_action(csp, c);
	csp.parts[csp.part_count] = bak;
}

static void set_action(CellStringParts csp, Cell *c) {
	(void)c;
	u16 addr;
	if(csp.part_count == 2) {
		if(parse_hex_16(csp.parts[1], &addr)) {
			pointer = addr;
			update_prefix();
			free(c->prefix);
			c->prefix = strdup(prefix);
		}
	} else {
		perr("Wrong arguments!");
		phgrn("\n[Usage] ", "set <16-bit address>");
	}
}

static void asm_exit(CellStringParts csp, Cell *c1) {
	(void)csp;
	c1->run = 0;
}

// clang-format off
static const char* asm_help =  
            "'asm' is the inline compilation shell of The8085."
            "\nIt can be invoked like the following : "
            "\n" hcode(asm) "<address>"
            "\nwhich will open the compilation shell with memory pointer set to <address>."
            "\nYou can write any valid 8085 opcode there to have it compiled to the location specified at"
            "\nthe line prefix. To write a statement starting with a label, prepend it the keyword 'label'"
            "\nlike the following : "
            "\n" hcode(jnz) "alabel"
            "\n" hcode(...)
            "\n" hcode(label) "alabel: hlt"
            "\nThe memory pointer will only increase if the result of the compilation was successful."
            "\nHowever if the compilation was unsuccessful, appropiate error messages will be printed"
            "\nand no guarantees will be made on the content of memory at prefix address."
            "\nTo set the memory pointer to any desired address, type '" hkw(set) " <16-bit address>'."
            "\nTo exit from that shell, type '" hkw(exit) "'. For help on the opcodes, type '" hkw(help) "'"
            "\nor '" hkw(help) " <opcode>'.";

// Since it's not final, let's hide it behind a switch
#ifdef ENABLE_OPCODE_HELP

#define il(x)   "\nInstruction Length : " #x " byte(s)"
#define mc(x,...)   "\nMachine cycle(s) : " #x ##__VA__ARGS__
#define am(x)   "\nAddressing Mode : " #x

/*  Opcode      Operand     I/L (Bytes)     M/C     T/S     Hex     A/M (Source, Destination)
 *  ======      =======     ===========     ===     ===     ===     =========================
 *
 *
 *
 */


#define op_m 0x1
#define op_r 0x2
#define op_8 0x4
#define op_16 0x8
#define op_sp 0x20
#define op_psw 0x10

typedef enum{
    AM_r,
    AM_ri,
    AM_d,
    AM_i,
    AM_im,
    AM_ipl
} AddrMode;

#define fl_s 0x1
#define fl_z 0x2
#define fl_a 0x4
#define fl_p 0x8
#define fl_c 0x10
#define fl_all 0x20

typedef struct{
    u32 operand;    // upto 4 operand types
    u8 il;
    u32 mc;         // upto 4 different mcs
    u32 ts;         // upto 4 different ts
    u8 flags;
    AddrMode ams, amd;
    const char *desc;
} InstructionFormat;

#define frmi(op, i, m, t, fl, as, ad, s)   \
    {.operand = op, .il = i, .mc = m, .ts = t, .flags = fl, .ams = AM_##as, .amd = AM_##ad, .desc = s}

static InstructionFormat opcodes_help[] = {
    frmi(op_8       , 2, 2, 7, fl_all, im, ipl, NULL),                      // ACI
    frmi(op_r | op_m, 1, 1 | (2 << 8), 4 | (7 << 8), fl_all, r, ipl, NULL), // ADC
    frmi(op_8       , 2, 2, 7, fl_all, im, ipl, NULL),                      // ADI
    frmi(op_r | op_m, 1, 1 | (2 << 8), 4 | (7 << 8), fl_all, r, ipl, NULL), // ANA
    frmi(op_8       , 2, 2, 7, fl_all, r, ipl, NULL),                       // ANI
    frmi(op_16      , 3, 3, 18, 0, im, ipl, NULL),                          // CALL
    frmi(op_16      , 3, 2 | (5 << 8), 9 | (18 << 8), 0, im, ipl, NULL)     // CC
};

#endif

// clang-format on

static void asm_action(CellStringParts csp, Cell *t) {
	(void)t;
	if(csp.part_count > 1) {
		u16 start;
		if(parse_hex_16(csp.parts[1], &start)) {
			compiler_reset();
			pointer = start;
			update_prefix();
			Cell editor = cell_init(prefix);
			u8   i      = 0;
#define INSTRUCTION(name, length) \
	cell_add_keyword(&editor, #name, descriptions[i++], parse_action);
#include "instruction.h"
#undef INSTRUCTION
			cell_add_keyword(&editor, "exit", "Exit from the assembler",
			                 asm_exit);
			cell_add_keyword(&editor, "help", "Show this help",
			                 cell_default_help);
			cell_add_keyword(&editor, "label", "Declare a label", label_action);
			cell_add_keyword(&editor, "set",
			                 "Set the memory pointer to the specified address",
			                 set_action);

			cell_repl(&editor);
			compiler_report_pending();
			cell_destroy(&editor);
		}
	} else {
		perr("Wrong arguments!");
		phgrn("\n[Usage] ", "asm <16-bit address>");
	}
}

void asm_init(Cell *cell, u8 *mem) {
	CellKeyword casm =
	    cell_create_keyword("asm", "Invoke the assembler", asm_action);
	casm.longhelp = asm_help;
	cell_insert_keyword(cell, casm);
	memory = mem;
}
