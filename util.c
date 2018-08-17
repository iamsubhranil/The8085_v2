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
