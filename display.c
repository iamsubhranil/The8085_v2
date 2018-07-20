#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

#include "display.h"

#ifdef DEBUG
#define setname(name) \
    void d##name(const char *msg, ...) {
#else
#define setname(name) \
    void p##name(const char *msg, ...) {
#endif

#define display(name, color, text) \
    setname(name) \
        printf(ANSI_FONT_BOLD); \
        printf(ANSI_COLOR_##color "\n" text " "); \
        printf(ANSI_COLOR_RESET); \
        va_list args; \
        va_start(args, msg); \
        vprintf(msg, args); \
        va_end(args); \
    }

display(dbg, GREEN, "[Debug]")
display(info, BLUE, "[Info]")
display(err, RED, "[Error]")
display(warn, YELLOW, "[Warning]")


#define print(name, color) \
    void p##name(const char* msg, ...){ \
        printf(ANSI_COLOR_##color); \
        va_list args; \
        va_start(args, msg); \
        vprintf(msg, args); \
        va_end(args); \
        printf(ANSI_COLOR_RESET); \
    }

print(red, RED)
print(blue, BLUE)
print(grn, GREEN)
print(ylw, YELLOW)
print(cyn, CYAN)
print(mgn, MAGENTA)

#define printh(name, color) \
    void ph##name(const char *header, const char* msg, ...){ \
        printf(ANSI_COLOR_##color); \
        printf("%s", header); \
        printf(ANSI_COLOR_RESET); \
        va_list args; \
        va_start(args, msg); \
        vprintf(msg, args); \
        va_end(args); \
    }

printh(red, RED)
printh(blue, BLUE)
printh(grn, GREEN)
printh(ylw, YELLOW)
printh(cyn, CYAN)
printh(mgn, MAGENTA)
