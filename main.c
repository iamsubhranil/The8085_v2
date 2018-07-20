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
    print_machine(&m);
    
    u8 memory[1 << 15];
    u16 pointer = 0;
    u16 *p = &pointer;

    const char *source =    "xra a\n"
                            "mvi b, 00h\n"
                            "loop: inr b\n"
                            "mov a, b\n"
                            "cpi 0ffh\n"
                            "jnz loop\n"
                            "hlt\n";

    CompilationStatus stat = compile(source, &memory[0], 1 << 15, p); 

    if(stat != COMPILE_OK){
        perr("Error occurred while compilation. Unable to run!");
        return 1;
    }
    run(&m, &memory[0]);

    print_machine(&m);
}
