#include <stdio.h>

#include "display.h"
#include "vm.h"
#include "bytecode.h"
#include "compiler.h"

int main(){
    Machine m;
    m.pc = m.sp = 0;
    for(u8 i = 0;i < 8; i++){
        m.registers[i] = 0;
    }
    
    u8 memory[1 << 15];
    u16 pointer = 0;
    u16 *p = &pointer;

    const char *source =    "mvi h, 10h\n"
                            "mvi l, 10h\n"
                            "mvi m, 19h\n"
                            "ldax h\n"
                            "cpi 19h\n"
                            "hlt\n";

    pinfo("Source");
    pinfo("======\n");
    printf("%s", source);

    pinfo("Compiling source");
    pinfo("================");
    CompilationStatus stat = compile(source, &memory[0], 1 << 15, p); 

    if(stat != COMPILE_OK){
        perr("Error occurred while compilation. Unable to run!");
        return 1;
    }

    pinfo("Disassembling compiled chunk");
    pinfo("============================\n");
    bytecode_disassemble_chunk(memory, 0);

    printf("\n\n");
    pinfo("Running chunk");
    pinfo("=============\n");
    run(&m, &memory[0]);
}
