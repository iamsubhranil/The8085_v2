#include <stdio.h>

#include "bytecode.h"
#include "calibrate.h"
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
    run(&machine, &memory[0], 0);
    if(!machine.isbroken){
        phgrn("\n[exec]", " Execution completed!");
    }
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
    pgrn("\n[show]");
    pred(" %x: ", addr);
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
    phgrn("\n[set]"," Old value : 0x%x", memory[addr]);
    memory[addr] = val;
    phgrn("\n[set]"," New value : 0x%x", memory[addr]);
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
            perr("Number of used labels exceeded the maximum permissible value!");
            perr("Compilation aborted!");
            goto load_action_cleanup;
        case LABELS_PENDING:
            perr("Not all used labels are declared in the program!");
            perr("Compilation aborted!");
            goto load_action_cleanup;
        case PARSE_ERROR:
            perr("Compilation aborted!");
            goto load_action_cleanup;
        case MEMORY_FULL:
            perr("Bytecode offset exceeded its size! Try loading the program at a lower address!");
            perr("Compilation aborted!");
            goto load_action_cleanup;
        default:
            phgrn("\n[load]"," '%s' loaded " ANSI_FONT_BOLD "[0x%x - 0x%x]", parts.parts[1], addr, memory_pointer - 1);
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

void brk_action(CellStringParts parts, Cell *cell){
    (void)cell;
    perr("Wrong arguments");
    pinfo("See 'help break'");
    cell_stringparts_free(parts);
}

void brkadd_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 addr;
    if(parts.part_count < 2){
        perr("Wrong number of arguments!");
        goto brkadd_usage;
    }
    if(!parse_address(parts.parts[1], &addr))
        goto brkadd_usage;
    if(!machine_add_breakpoint(&machine, addr)){
        perr("Maximum number(%d) of breakpoints already set!", MAX_BREAKPOINT_COUNT);
    }
    else{
        phgrn("\n[break add]"," Breakpoint added at address 0x%x", addr);
    }
brkadd_cleanup:
    cell_stringparts_free(parts);
    return;
brkadd_usage:
    phgrn("\n[Usage]", " break add <16-bit address>");
    goto brkadd_cleanup;
}

void cont_action(CellStringParts cp, Cell *cell){
    (void)cp; (void)cell;
    if(machine.isbroken){
        machine.isbroken = 0;
        run(&machine, &memory[0], 0);
        if(!machine.isbroken){
            phgrn("\n[continue]", " Execution completed!");
        }
    }
    else{
        perr("No program is running! Unable to continue!");
    }
    cell_stringparts_free(cp);
}

void step_action(CellStringParts cp, Cell *cell){
    (void)cp; (void)cell;
    if(machine.isbroken){
        run(&machine, &memory[0], 1);
        if(!machine.isbroken){
            phgrn("\n[step]"," Execution completed!");
        }
    }
    else{
        perr("No program is running! Unable to step!");
    }
    cell_stringparts_free(cp);
}

void brkview_action(CellStringParts cp, Cell *cell){
    (void)cell;
    if(machine.breakpoint_pointer == 0)
        pinfo("No breakpoints attached!");
    else{
        for(u16 i = 0;i < machine.breakpoint_pointer;i++){
            pylw("\n[Breakpoint %" Pu16 "]", i);
            printf(" 0x%x", machine.breakpoints[i]);
        }
    }
    cell_stringparts_free(cp);
}

void brkrm_action(CellStringParts cp, Cell *cell){
    (void)cell;
    u16 addr;
    if(cp.part_count < 2){
        perr("Wrong arguments!");
        goto brkrm_usage;
    }
    if(!parse_address(cp.parts[1], &addr))
        goto brkrm_usage;
    if(machine.breakpoint_pointer == 0){
        pwarn("No breakpoints attached!");
    }
    else if(!machine_remove_breakpoint(&machine, addr)){
        perr("No such breakpoint found!");
    }
    else{
        phgrn("\n[break remove]"," Breakpoint removed from 0x%x", addr);
    }
brkrm_cleanup:
    cell_stringparts_free(cp);
    return;
brkrm_usage:
    phgrn("\n[Usage]", " break remove <16-bit address>");
    goto brkrm_cleanup;
}

void calb_action(CellStringParts csp, Cell *c){
    (void)c;
    calibrate(&machine);
    cell_stringparts_free(csp);
}

void exit_action(CellStringParts parts, Cell *cell){
    cell_stringparts_free(parts);
    cell->run = 0;
}

static void init_machine(){
    machine.pc = 0;
    machine.sp = 0xffff;
    machine.breakpoint_pointer = 0;
    machine.isbroken = 0;
    machine.issilent = 0;
    machine.sleepfor.tv_nsec = 0;
}

int main(){
    dump_init();
    init_machine();
#ifdef ENABLE_TESTS
    test_all();
#endif
    Cell cell = cell_init(ANSI_FONT_BOLD ">>" ANSI_COLOR_RESET);
    CellKeyword exec = cell_create_keyword("exec", ANSI_COLOR_GREEN, "Execute the instructions from the specified address", exec_action);
    CellKeyword show = cell_create_keyword("show", ANSI_COLOR_GREEN, "Show the contents of a specific address", show_action);
    CellKeyword set = cell_create_keyword("set", ANSI_COLOR_GREEN, "Set the content of a address to a specific value", set_action);
    CellKeyword load = cell_create_keyword("load", ANSI_COLOR_GREEN, "Compile and load a program at the given address", load_action);
    CellKeyword ext = cell_create_keyword("exit", ANSI_COLOR_GREEN, "Exit from the REPL", exit_action);
    CellKeyword help = cell_create_keyword("help", ANSI_COLOR_GREEN, "Show this help", cell_default_help);
    CellKeyword dis = cell_create_keyword("dis", ANSI_COLOR_GREEN, "Disassemble bytes stored between specified addresses", dis_action);
    CellKeyword brk = cell_create_keyword("break", ANSI_COLOR_GREEN, 
            "Manage breakpoints", brk_action);
    CellKeyword cont = cell_create_keyword("continue", ANSI_COLOR_GREEN,
            "Continue execution till the next breakpoint", cont_action);
    CellKeyword step = cell_create_keyword("step", ANSI_COLOR_GREEN,
            "Step through the execution of the program", step_action);
    CellKeyword brkview = cell_create_keyword("view", ANSI_COLOR_GREEN,
            "View all attached breakpoints", brkview_action);
    CellKeyword brkadd = cell_create_keyword("add", ANSI_COLOR_GREEN,
            "Add a new breakpoint at the given address", brkadd_action);
    CellKeyword brkrem = cell_create_keyword("remove", ANSI_COLOR_GREEN,
            "Remove an attached breakpoint from the given address", brkrm_action);
    CellKeyword calb = cell_create_keyword("calibrate", ANSI_COLOR_GREEN, 
            "Calibrate the virtual machine to better sync with the host", calb_action);
    cell_add_subkeyword(&brk, brkview);
    cell_add_subkeyword(&brk, brkadd);
    cell_add_subkeyword(&brk, brkrem);
    cell_insert_keyword(&cell, exec);
    cell_insert_keyword(&cell, show);
    cell_insert_keyword(&cell, set);
    cell_insert_keyword(&cell, load);
    cell_insert_keyword(&cell, ext);
    cell_insert_keyword(&cell, help);
    cell_insert_keyword(&cell, dis);
    cell_insert_keyword(&cell, brk);
    cell_insert_keyword(&cell, cont);
    cell_insert_keyword(&cell, step);
    cell_insert_keyword(&cell, calb);
    cell_repl(&cell);
    cell_destroy(&cell);
    printf("\n");
    return 0;
}
