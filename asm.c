#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm.h"
#include "compiler.h"
#include "display.h"
#include "util.h"

static u8 *memory = NULL;
static u16 pointer = 0;
static char prefix[30] = {0}; //  [c050] -->
static const char* descriptions[] = {
    "Add immediate to accumulator with carry",              // ACI
    "Add register or memory to accumulator with carry",     // ADC
    "Add register or memory to accumulator",                // ADD
    "Add immediate to accumulator",                         // ADI
    "Logical AND register or memory with accumulator",      // ANA
    "Logical AND immediate with accumulator",               // ANI
    "Call a subroutine unconditionally",                    // CALL
    "Call on carry (C = 1)",                                // CC
    "Call on minus (S = 1)",                                // CM
    "Complement accumulator",                               // CMA
    "Complement carry",                                     // CMC
    "Compare with accumulator",                             // CMP
    "Call on no carry (C = 0)",                             // CNC
    "Call on no zero (Z = 0)",                              // CNZ
    "Call on positive (S = 0)",                             // CP
    "Call on parity even (P = 1)",                          // CPE
    "Compare immediate with accumulator",                   // CPI
    "Call on parity odd (P = 0)",                           // CPO
    "Call on zero (Z = 1)",                                 // CZ
    "Decimal adjust accumulator",                           // DAA
    "Add register pair to H and L registers",               // DAD
    "Decrement content of register or memory by 1",         // DCR
    "Decrement content of register pair by 1",              // DCX
    "Halt and enter wait state",                            // HLT
    "Input data to accumulator from the specified port",    // IN
    "Increment content of register or memory by 1",         // INR
    "Increment content of register pair by 1",              // INX
    "Jump on carry (C = 1)",                                // JC
    "Jump on minus (S = 1)",                                // JM
    "Jump unconditionally",                                 // JMP
    "Jump on no carry (C = 0)",                             // JNC
    "Jump on no zero (Z = 0)",                              // JNZ
    "Jump on positive (S = 0)",                             // JP
    "Jump on parity even (P = 1)",                          // JPE
    "Jump on parity odd (P = 0)",                           // JPO
    "Jump on zero (Z = 1)",                                 // JZ
    "Load accumulator direct",                              // LDA
    "Load accumulator indirect",                            // LDAX
    "Load H and L registers direct",                        // LHLD
    "Load register pair immediate",                         // LXI
    "Move (copy) from source to destination",               // MOV
    "Move immediate",                                       // MVI
    "No operation",                                         // NOP
    "Logical OR register or memory with accumulator",       // ORA
    "Logical OR immediate with accumulator",                // ORI
    "Output data from accumulator to the specified port",   // OUT
    "Load program counter with HL contents",                // PCHL
    "Pop off top of the stack to a register pair",          // POP
    "Push a register pair to top of the stack",             // PUSH
    "Rotate accumulator left through carry",                // RAL
    "Rotate accumulator right through carry",               // RAR
    "Return on carry (C = 1)",                              // RC
    "Return unconditionally",                               // RET
    "Rotate accumulator left",                              // RLC
    "Return on minus (S = 1)",                              // RM
    "Return on no carry (C = 0)",                           // RNC
    "Return on no zero (Z = 0)",                            // RNZ
    "Return on positive (S = 0)",                           // RP
    "Return on parity even (P = 1)",                        // RPE
    "Return on parity odd (P = 0)",                         // RPO
    "Rotate accumulator right",                             // RRC
    "Return on zero (Z = 1)",                               // RZ
    "Subtract register or memory from accumulator with borrow",   // SBB
    "Subtract immediate from accumulator with borrow",      // SBI
    "Store H and L registers direct",                       // SHLD
    "Copy H and L registers to the stack pointer",          // SPHL
    "Store accumulator direct",                             // STA
    "Store accumulator indirect",                           // STAX
    "Set carry",                                            // STC
    "Subtract register or memory from accumulator",         // SUB
    "Subtract immediate from accumulator",                  // SUI
    "Exchange H and L with D and E",                        // XCHG
    "Exclusive OR register or memory with accumulator",     // XRA
    "Exclusive OR immediate with accumulator",              // XRI
    "Exchange H and L with top of the stack",               // XTHL
};


static void update_prefix(){
    sprintf(prefix, ANSI_FONT_BOLD "[0x%04x] >>" ANSI_COLOR_RESET, pointer);
}

static void parse_action(CellStringParts csp, Cell *cell){
    char *str = NULL;
    siz size = 0, bak = 0;
    for(siz i = 0;i < csp.part_count; i++){
        bak = size;
        size = strlen(csp.parts[i]) + 1;
        str = (char *)realloc(str, bak + size);
        memcpy(&str[bak], csp.parts[i], size);
        size += bak;
        str[size - 1] = ' ';
    }
    str = (char *)realloc(str, size + 1);
    str[size] = 0;
    u16 pbak = pointer;
    CompilationStatus res = compile(str, memory, 0xffff, &pointer);
    switch(res){
        case PARSE_ERROR:
        case LABEL_FULL:
        case MEMORY_FULL:
            perr("Compilation aborted!");
            pointer = pbak;
            break;
        case LABELS_PENDING:
            pinfo("To avoid erroneous results, either declare or manually patch the label before execution.");
        case COMPILE_OK:
            update_prefix();
            free(cell->prefix);
            cell->prefix = strdup(prefix);
            for(u16 i = pbak;i < pointer;i++){
                pred("\n0x%04x: ", i);
                printf("0x%02x", memory[i]);
            }
            break;
    }
    cell_stringparts_free(csp);
    free(str);
}

static void label_action(CellStringParts csp, Cell *c){
    free(csp.parts[0]);
    for(siz i = 0;i < csp.part_count - 1;i++){
        csp.parts[i] = csp.parts[i + 1];
    }
    csp.part_count--;
    parse_action(csp, c);
}

static void set_action(CellStringParts csp, Cell *c){
    (void)c;
    u16 addr;
    if(csp.part_count == 2){
        if(parse_hex_16(csp.parts[1], &addr)){
            pointer = addr;
            update_prefix();
            free(c->prefix);
            c->prefix = strdup(prefix);
        }
    }
    else{
        perr("Wrong arguments!");
        phgrn("\n[Usage] ", "set <16-bit address>");
    }
    cell_stringparts_free(csp);
}

static void asm_exit(CellStringParts csp, Cell *c1){
    c1->run = 0;
    cell_stringparts_free(csp);
}

static void asm_action(CellStringParts csp, Cell *t){
    (void)t;
    if(csp.part_count > 1){
        u16 start;
        if(parse_hex_16(csp.parts[1], &start)){
            compiler_reset();
            pointer = start;
            update_prefix();
            Cell editor = cell_init(prefix);
            u8 i = 0;
            #define INSTRUCTION(name, length)                                                       \
                cell_add_keyword(&editor, #name, ANSI_COLOR_GREEN, descriptions[i++], parse_action);
            #include "instruction.h"
            #undef INSTRUCTION
            cell_add_keyword(&editor, "exit", ANSI_COLOR_GREEN, "Exit from the assembler", asm_exit);
            cell_add_keyword(&editor, "help", ANSI_COLOR_GREEN, "Show this help", cell_default_help);
            cell_add_keyword(&editor, "label", ANSI_COLOR_GREEN, "Declare a label", label_action);
            cell_add_keyword(&editor, "set", ANSI_COLOR_GREEN, "Set the memory pointer to the specified address", set_action);
            
            pinfo("Welcome to the assembler!");
            pinfo("You can write any valid 8085 opcode here to have it compiled to the location specified at");
            pinfo("the line prefix. To write a statement starting with a label, prepend it the keyword 'label',");
            pinfo("like the following : \n");
            pinfo("         jnz alabel");
            pinfo("         ...");
            pinfo("         label alabel: hlt\n");
            pinfo("The memory pointer will only increase if the result of the compilation was successful.");
            pinfo("To set the memory pointer to any desired address, type 'set <16-bit address>'.");
            pinfo("To exit from this shell, type 'exit'. For help on the keywords, type 'help'.");

            cell_repl(&editor);
            compiler_report_pending();
            cell_destroy(&editor);
        }
    }
    else{
        perr("Wrong arguments!");
        phgrn("\n[Usage] ", "asm <16-bit address>"); 
    }
    cell_stringparts_free(csp);
}

void asm_init(Cell *cell, u8 *mem){
    CellKeyword casm = cell_create_keyword("asm", ANSI_COLOR_GREEN, "Invoke the assembler", asm_action);
    cell_insert_keyword(cell, casm);
    memory = mem;
}
