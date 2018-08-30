#include <stdio.h>

#include "common.h"
#include "display.h"
#include "util.h"

char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");

    if (file == NULL) {
        perr("Could not open file \"%s\".\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        perr("Not enough memory to read \"%s\".\n", path);
        return NULL;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        perr("Could not read file \"%s\".\n", path);
        return NULL;
    } 
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

bool parse_hex_byte(const char *str, u8 *store){
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

bool parse_hex_16(const char *str, u16 *store){
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
