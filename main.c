#include <stdio.h>

#include "bytecode.h"
#include "compiler.h"
#include "Cell/cell.h"
#include "display.h"
#include "dump.h"
#include "test.h"
#include "util.h"
#include "vm.h"

// State
static Machine machine;
static u8 memory[0xffff] = {0};
static u16 memory_pointer = 0;

bool parse_byte(char *str, u8 *store){
    char *end = NULL;
    u32 val;
    val = strtol(str, &end, 16);
    if(*end != 0){
        perr("Bad byte '%s'!", str);
        return 0;
    }
    if(val > 0xff){
        perr("Byte out of range : '%s'!", str);
        return 0;
    }
    *store = val;
    return 1;
}


bool parse_address(char *str, u16 *store){
    char *end = NULL;
    u32 addr;
    addr = strtol(str, &end, 16);
    if(*end != 0){
        perr("Bad address '%s'!", str);
        return 0;
    }
    if(addr > 0xffff){
        perr("Address out of range : '%s'!", str);
        return 0;
    }
    *store = addr;
    return 1;
}

void exec_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 from;
    if(parts.part_count < 2){
        perr("Specify the address to execute from!");
        goto exec_usage;
    }
    if(!parse_address(parts.parts[1], &from))
        goto exec_usage;
    phgrn("\n[exec]", " Executing from 0x%x ", from);
    fflush(stdout);
    machine.pc = from;
    run(&machine, &memory[0]);
exec_action_cleanup:
    cell_stringparts_free(parts);
    return;
exec_usage:
    phgrn("\n[Usage]", " exec <16-bit memory address>");
    goto exec_action_cleanup;
}

void show_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 addr;
    if(parts.part_count < 2){
        perr("Specify the address to inspect!");
        goto show_usage;
    }
    if(!parse_address(parts.parts[1], &addr))
        goto show_usage;
    //phgrn("\n[show]", " Showing the content of 0x%x", addr);
    pred("\n%x: ", addr);
    printf("0x%x", memory[addr]);
show_action_cleanup:
    cell_stringparts_free(parts);
    return;
show_usage:
    phgrn("\n[Usage]", " show <16-bit memory address>");
    goto show_action_cleanup;
}

void set_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 addr;
    u8 val;
    if(parts.part_count < 3){
        perr("Wrong arguments!");
        goto set_usage;
    }
    if(!parse_address(parts.parts[1], &addr))
        goto set_usage;
    if(!parse_byte(parts.parts[2], &val))
        goto set_usage;
    //phgrn("\n[set]", " Setting the content of 0x%x to 0x%x", addr, val);
    printf("\nOld value : 0x%x", memory[addr]);
    memory[addr] = val;
    printf("\nNew value : 0x%x", memory[addr]);
set_action_cleanup:
    cell_stringparts_free(parts);
    return;
set_usage:
    phgrn("\n[Usage]", " set <16-bit memory address> <8-bit value>");
    goto set_action_cleanup;
}

void load_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 addr;
    char *source = NULL;
    CompilationStatus stat = COMPILE_OK;
    if(parts.part_count < 3){
        perr("Wrong number of arguments!");
        goto load_usage;
    }
    if(!parse_address(parts.parts[2], &addr))
        goto load_usage;
    //phgrn("\n[Load]", " Compiling and loading %s at 0x%x", parts.parts[1], addr);
    source = readFile(parts.parts[1]);
    if(source == NULL)
        goto load_action_cleanup;
    memory_pointer = addr;
    compiler_reset();
    stat = compile(source, &memory[0], 0xffff, &memory_pointer);
    switch(stat){
        case LABEL_FULL:
            perr("Compilation aborted!");
            perr("Number of used labels exceeded the maximum permissible value!");
            goto load_action_cleanup;
        case LABELS_PENDING:
            perr("Compilation aborted!");
            perr("Not all used labels are declared in the program!");
            goto load_action_cleanup;
        case PARSE_ERROR:
            perr("Compilation aborted!");
            goto load_action_cleanup;
        case MEMORY_FULL:
            perr("Compilation aborted!");
            perr("Bytecode offset exceeded its size! Try loading the program at a lower address!");
            goto load_action_cleanup;
        default:
            pinfo("'%s' loaded in memory in address range 0x%x-0x%x", parts.parts[1], addr, memory_pointer - 1);
            break;
    }
load_action_cleanup:
    cell_stringparts_free(parts);
    return;
load_usage:
    phgrn("\n[Usage]", " load <filename> <16-bit memory address>");
    goto load_action_cleanup;
}

void dis_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 strtaddr, endaddr;
    if(parts.part_count < 3){
        perr("Wrong number of arguments!");
        goto dis_usage;
    }
    if(!parse_address(parts.parts[1], &strtaddr))
        goto dis_usage;
    if(!parse_address(parts.parts[2], &endaddr))
        goto dis_usage;
    //phgrn("\n[dis]", " Disassembling memory contents from 0x%x to 0x%x", strtaddr, endaddr);
    bytecode_disassemble_chunk(&memory[0], strtaddr, endaddr);
dis_action_cleanup:
    cell_stringparts_free(parts);
    return;
dis_usage:
    phgrn("\n[Usage]", " dis <starting address> <ending address>");
    goto dis_action_cleanup;
}

void exit_action(CellStringParts parts, Cell *cell){
    cell_stringparts_free(parts);
    cell->run = 0;
}

static void init_machine(){
    machine.pc = 0;
    machine.sp = 0xffff;
}

int main(){
#ifdef USE_NEOVM
    dump_init();
    //test_all();
#endif
    init_machine();
    Cell cell = cell_init(ANSI_FONT_BOLD ">>" ANSI_COLOR_RESET);
    CellKeyword exec = cell_create_keyword("exec", ANSI_COLOR_GREEN, "Execute the instructions from the specified address", exec_action);
    CellKeyword show = cell_create_keyword("show", ANSI_COLOR_GREEN, "Show the contents of a specific address", show_action);
    CellKeyword set = cell_create_keyword("set", ANSI_COLOR_GREEN, "Set the content of a address to a specific value", set_action);
    CellKeyword load = cell_create_keyword("load", ANSI_COLOR_GREEN, "Compile and load a program at the given address", load_action);
    CellKeyword ext = cell_create_keyword("exit", ANSI_COLOR_GREEN, "Exit from the REPL", exit_action);
    CellKeyword help = cell_create_keyword("help", ANSI_COLOR_GREEN, "Show this help", cell_default_help);
    CellKeyword dis = cell_create_keyword("dis", ANSI_COLOR_GREEN, "Disassemble bytes stored between specified addresses", dis_action);
    cell_insert_keyword(&cell, exec);
    cell_insert_keyword(&cell, show);
    cell_insert_keyword(&cell, set);
    cell_insert_keyword(&cell, load);
    cell_insert_keyword(&cell, ext);
    cell_insert_keyword(&cell, help);
    cell_insert_keyword(&cell, dis);
    cell_repl(&cell);
    cell_destroy(&cell);
    printf("\n");
    return 0;
}
