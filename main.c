#include <stdio.h>

#include "display.h"
#include "vm.h"
#include "bytecode.h"
#include "compiler.h"

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    } 
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

int main(int argc, char *argv[]){
    Machine m;
    m.pc = 0;
    m.sp = 0xffff - 1;
    for(u8 i = 0;i < 8; i++){
        m.registers[i] = 0;
    }
    
    u8 memory[0xffff];
    u16 pointer = 0;
    u16 *p = &pointer;

    if(argc == 1){
        perr("Usage : %s file_name\n", argv[0]);
        return 1;
    }

    char *source = readFile(argv[1]);

    pinfo("Source");
    pinfo("======\n");
    printf("%s", source);

    pinfo("Compiling source");
    pinfo("================");
    CompilationStatus stat = compile(source, &memory[0], 0xffff, p); 

    if(stat != COMPILE_OK){
        perr("Error occurred while compilation. Unable to run!");
        return 1;
    }

    pinfo("Disassembling compiled chunk");
    pinfo("============================\n");
    bytecode_disassemble_chunk(memory, 0, pointer);

    printf("\n\n");
    pinfo("Running chunk");
    pinfo("=============\n");
    run(&m, &memory[0]);

    printf("\n");
    return 0;
}
