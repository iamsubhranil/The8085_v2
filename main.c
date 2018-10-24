#include <stdio.h>
#include <string.h>

#include "asm.h"
#include "bytecode.h"
#include "calibrate.h"
#include "compiler.h"
#include "cosmetic.h"
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

void exec_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 from;
    if(parts.part_count > 1){
        if(parse_hex_16(parts.parts[1], &from)){
            phgrn("\n[exec]", " Executing from 0x%x ", from);
            fflush(stdout);
            machine.pc = from;
            run(&machine, &memory[0], 0);
            if(!machine.isbroken){
                phgrn("\n[exec]", " Execution completed!");
            }
            return;
        }
    }
    else
        perr("Specify the address to execute from!");
    phgrn("\n[Usage]", " exec <16-bit memory address>");
}

void show_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 addr;
    if(parts.part_count > 1){
        if(parse_hex_16(parts.parts[1], &addr)){
            pgrn("\n[show]");
            pred(" %x: ", addr);
            printf("0x%x", memory[addr]);
            return;
        }
    }
    else
        perr("Specify the address to inspect!");
    phgrn("\n[Usage]", " show <16-bit memory address>");
}

void set_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 addr;
    u8 val;
    if(parts.part_count > 2){
        if(parse_hex_16(parts.parts[1], &addr) && parse_hex_byte(parts.parts[2], &val)){
            phgrn("\n[set]"," Old value : 0x%x", memory[addr]);
            memory[addr] = val;
            phgrn("\n[set]"," New value : 0x%x", memory[addr]);
            return;
        }
    }
    else
        perr("Wrong arguments!");
    phgrn("\n[Usage]", " set <16-bit memory address> <8-bit value>");
}

static int load_successful = 0;

void load_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 addr;
    char *source = NULL;
    CompilationStatus stat = COMPILE_OK;
    if(parts.part_count > 2 && parse_hex_16(parts.parts[2], &addr)){
        source = readFile(parts.parts[1]);
        if(source != NULL){
            memory_pointer = addr;
            compiler_reset();
            load_successful = 0;
            stat = compile(source, &memory[0], 0xffff, &memory_pointer);
            switch(stat){
                case LABEL_FULL:
                    perr("Number of used labels exceeded the maximum permissible value!");
                    perr("Compilation aborted!");
                    break;
                case LABELS_PENDING:
                    perr("Not all used labels are declared in the program!");
                    perr("Compilation aborted!");
                    break;
                case PARSE_ERROR:
                    perr("Compilation aborted!");
                    break;
                case MEMORY_FULL:
                    perr("Bytecode offset exceeded its size! Try loading the program at a lower address!");
                    perr("Compilation aborted!");
                    break;
                case EMPTY_PROGRAM:
                    perr("Program contains no valid instructions!");
                    perr("Compilation aborted!");
                    break;
                case NO_HLT:
                    perr("Program does not contain any halt instruction!");
                    perr("This will surely result in an infinite loop!");
                    perr("Fix this, and then recompile the program.");
                    break;
                default:
                    phgrn("\n[load]"," '%s' loaded " ANSI_FONT_BOLD "[0x%x - 0x%x]", parts.parts[1], addr, memory_pointer - 1);
                    load_successful = 1;
                    break;
            }
            free(source);
            return;
        }
    }
    else
        perr("Wrong number of arguments!");
    phgrn("\n[Usage]", " load <filename> <16-bit memory address>");
}

void dis_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 strtaddr, endaddr;
    char separator[45] = {[0 ... 44] = '-'};
    separator[44] = 0;
    if(parts.part_count > 2){
        if(parse_hex_16(parts.parts[1], &strtaddr) && parse_hex_16(parts.parts[2], &endaddr)){
            printf("\nAddress %15s\t\t%8s", "Assembly", "Hex");
            printf("\n%s", separator);
            bytecode_disassemble_chunk(&memory[0], strtaddr, endaddr);
            return;
        }
    }
    else
        perr("Wrong number of arguments!");
    phgrn("\n[Usage]", " dis <starting address> <ending address>");
}

void brk_action(CellStringParts parts, Cell *cell){
    (void)cell; (void)parts;
    perr("Wrong arguments");
    pinfo("See 'help break'");
}

void brkadd_action(CellStringParts parts, Cell *cell){
    (void)cell;
    u16 addr;
    if(parts.part_count > 1){
        if(parse_hex_16(parts.parts[1], &addr)){
            if(!machine_add_breakpoint(&machine, addr)){
                perr("Maximum number(%d) of breakpoints already set!", MAX_BREAKPOINT_COUNT);
            }
            else{
                phgrn("\n[break add]"," Breakpoint added at address 0x%x", addr);
            }
            return;
        }
    }
    else
        perr("Wrong number of arguments!");
    phgrn("\n[Usage]", " break add <16-bit address>");
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
}

void brkview_action(CellStringParts cp, Cell *cell){
    (void)cell; (void)cp;
    if(machine.breakpoint_pointer == 0)
        pinfo("No breakpoints attached!");
    else{
        for(u16 i = 0;i < machine.breakpoint_pointer;i++){
            pylw("\n[Breakpoint %" Pu16 "]", i);
            printf(" 0x%x", machine.breakpoints[i]);
        }
    }
}

void brkrm_action(CellStringParts cp, Cell *cell){
    (void)cell;
    u16 addr;
    if(cp.part_count > 1){
        if(parse_hex_16(cp.parts[1], &addr)){
            if(machine.breakpoint_pointer == 0){
                pwarn("No breakpoints attached!");
            }
            else if(!machine_remove_breakpoint(&machine, addr)){
                perr("No such breakpoint found!");
            }
            else{
                phgrn("\n[break remove]"," Breakpoint removed from 0x%x", addr);
            }
            return;
        }
    }
    else
        perr("Wrong arguments!");
    phgrn("\n[Usage]", " break remove <16-bit address>");
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


// Descriptive help messages for the keywords
static const char *longhelp[] = {
        "'exec' can be used to execute some instructions stored in the memory. The machine will"
        "\ncontinue executing consecutive instructions until either of the following three cases"
        "\nhappened : "
        "\n1. The machine encounters a " hins(hlt) " instruction."
        "\n2. The program counter reaches a break point."
        "\n3. The execution was started using a " hkw(step) " keyword, in which case,"
        "\n   only one instruction will be executed and the contents of the registers"
        "\n   and flags will be shown to the user after execution."
        "\n" husage(exec) "c050"
        "\nThe above command will cause the machine to execute consecutive instructions"
        "\nstarting from memory address 0xc050.",
        "Using 'show', you can inspect (but not change) the content of a memory address."
        "\n" husage(show) "c050"
        "\nThe above command will print the byte stored at address 0xc050 in hexadecimal"
        "\nformat.",
        "To set the content of a specific memory address to a particular value, use 'set'"
        "\nlike the following : "
        "\n" husage(set) "c050 3f"
        "\nThe above will cause the value of memory address 0xc050 to be set to 0x3f.",
        "Given a file name and an address to store, invoking 'load' will cause the file"
        "\nto be compiled to valid 8085 bytecodes, and the bytecodes to be stored consecutively"
        "\nin memory starting from the given address."
        "\nIf the result of compilation was unsuccessful, appropiate messages are printed and"
        "\nno guarantees are made on the content of the consecutive memory locations starting"
        "\nfrom the given address."
        "\nThis is usually referred to as 'file mode compilation' of The8085. The8085 also offers"
        "\nanother mode of line by line compilation by using the " hkw(asm) " keyword. For"
        "\nmore information, see '" hkw(help) " asm'."
        "\n" husage(load) "test/loop.8085 c050"
        "\nWhen the above command is used, 'test/loop.8085' is read by The8085 (if possible),"
        "\ncompiled to original 8085 opcodes, and stored in consecutive memory locations"
        "\nstarting from 0xc050.",
        "'exit' will cause this REPL to release any dynamically allocated resources,"
        "\nstop the REPL loop itself and return back to the parent shell."
        "\n" husage(exit),
        "'help' shows informative messages about the keywords in this shell."
        "\nTo list all keywords of the shell, use : "
        "\n" hcode(help)
        "\nTo get a more descriptive message about a keyword and list its subcommands,"
        "\nuse : "
        "\n" hcode(help) "<keyword>"
        "\nTo get information about a subcommand of a keyword, use : "
        "\n" hcode(help) "<keyword> <subcommand>",
        "'dis' is The8085 disassembler. It reads original 8085 opcodes from memory,"
        "\nconverts the to assembly and prints them to the terminal."
        "\nThe first address given to 'dis' is the starting address of disassembly,"
        "\nbut 'dis' needs to know when to stop. Hence, for now, and explicit ending"
        "\naddress is needed as an argument to 'dis', but this might change in the"
        "\nfuture."
        "\n" husage(dis) "c050 c06a"
        "\nThe above will cause the disassembler to read and print all valid 8085"
        "\ninstructions starting from 0xc050 upto (including) 0xc06a in memory.",
        "'break' is The8085 breakpoint manager. You can add, remove or view"
        "\nbreakpoints using the subcommands shown below. For more information on a"
        "\nparticular subcommand, type : "
        "\n" hcode(help) "break <subcommand>",
        "After halting on a breakpoint, if you want to continue the execution"
        "\nof the instructions until either the next breakpoint or a " hins(hlt)
        "\noccurs, use 'continue'. Do remember though continue will only work"
        "\nif the machine has been stopped by either a breakpoint or by issuing"
        "\na " hkw(step) "."
        "\n" husage(continue),
        "After halting on a breakpoint, if you want the machine to execute only the next"
        "\ninstruction and then halt again, use 'step'."
        "\nThis will only work if the machine was halted on a breakpoint before."
        "\n" husage(step),
        "Use 'break view' to show all the attached breakpoints, sorted in the order of"
        "\ninsertion."
        "\n" husage(break) "view",
        "To add a breakpoint at an address, i.e. to halt the machine when the program"
        "\ncounter reaches a particular address, use 'break add' like the following : "
        "\n" hcode(break) "add <address>"
        "\nWhen the program counter reaches <address>, the machine will pause the execution,"
        "\nprint all of its registers and status flags, and wait for further user input"
        "\nto determine its execution. You can then either inspect memory (" hkw(set)
        "\nand " hkw(show) "), execute only the next instruction i.e. the instruction at "
        "\n<address> (" hkw(step) "), or continue execution until either the program ends"
        "\nor a breakpoint has occurred (" hkw(continue) ")."
        "\nAll other keywords of the shell will also remain fully valid at that state and"
        "\ntogether will constitute a powerful and robust debugging solution for the system."
        "\nIf you add more than one breakpoints at the same address, only the first will"
        "\nremain available.",
        "To remove a previously attached breakpoint by its address, use 'break remove'."
        "\n" husage(break) "remove <address>"
        "\nIf <address> was not previously attached as a breakpoint, an error message"
        "\nwill be shown.",
        "The host machine that The8085 is being executed on is way more powerful and fast"
        "\nthan an original 8085 chip. To manually slow down the execution of the virtual"
        "\nmachine, you can use 'calibrate', which will try to bound the execution to"
        "\n~3MHz. It is not perfect yet, and the only way to reset back to the original"
        "\nspeed of the host machine is by exit and reenter for now, so use with caution."
        "\n" husage(calibrate),
};

int main(int argc, char *argv[]){
#ifndef __AFL_COMPILER
    dump_init();
#endif
    init_machine();
#ifdef ENABLE_TESTS
    test_all();
#endif
    if(argc == 2){
        CellStringParts csp;
        csp.part_count = 3;
        csp.parts = (char **)malloc(sizeof(char*) * 3);
        pdbg("Filename : %s\n", argv[1]);
        csp.parts[1] = strdup(argv[1]);
        csp.parts[2] = strdup("0x0100");
        load_action(csp, NULL);
        free(csp.parts[1]);
        if(load_successful) {
            bytecode_disassemble_chunk(&memory[0], 0x0100, memory_pointer - 1);
            csp.part_count = 2;
            csp.parts[1] = csp.parts[2];
            exec_action(csp, NULL);
        }
        free(csp.parts[2]);
        free(csp.parts);
        return 0;
    }
    Cell cell = cell_init(ANSI_FONT_BOLD ">>" ANSI_COLOR_RESET);
    CellKeyword exec = cell_create_keyword("exec", "Execute the instructions from the specified address", exec_action);
    exec.longhelp = longhelp[0];
    CellKeyword show = cell_create_keyword("show", "Show the contents of a specific address", show_action);
    show.longhelp = longhelp[1];
    CellKeyword set = cell_create_keyword("set", "Set the content of a address to a specific value", set_action);
    set.longhelp = longhelp[2];
    CellKeyword load = cell_create_keyword("load", "Compile and load a program at the given address", load_action);
    load.longhelp = longhelp[3];
    CellKeyword ext = cell_create_keyword("exit", "Exit from the REPL", exit_action);
    ext.longhelp = longhelp[4];
    CellKeyword help = cell_create_keyword("help", "Show this help", cell_default_help);
    help.longhelp = longhelp[5];
    CellKeyword dis = cell_create_keyword("dis", "Disassemble bytes stored between specified addresses", dis_action);
    dis.longhelp = longhelp[6];
    CellKeyword brk = cell_create_keyword("break", 
            "Manage breakpoints", brk_action);
    brk.longhelp = longhelp[7];
    CellKeyword cont = cell_create_keyword("continue", 
            "Continue execution till the next breakpoint", cont_action);
    cont.longhelp = longhelp[8];
    CellKeyword step = cell_create_keyword("step", 
            "Step through the execution of the program", step_action);
    step.longhelp = longhelp[9];
    CellKeyword brkview = cell_create_keyword("view", 
            "View all attached breakpoints", brkview_action);
    brkview.longhelp = longhelp[10];
    CellKeyword brkadd = cell_create_keyword("add", 
            "Add a new breakpoint at the given address", brkadd_action);
    brkadd.longhelp = longhelp[11];
    CellKeyword brkrem = cell_create_keyword("remove", 
            "Remove an attached breakpoint from the given address", brkrm_action);
    brkrem.longhelp = longhelp[12];
    CellKeyword calb = cell_create_keyword("calibrate", 
            "Calibrate the virtual machine to better sync with the host", calb_action);
    calb.longhelp = longhelp[13];
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
    asm_init(&cell, &memory[0]);
    cell_repl(&cell);
    cell_destroy(&cell);
    compiler_reset();
    printf("\n");
    return 0;
}
