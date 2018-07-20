#pragma once

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_FONT_BOLD     "\x1b[1m"

#define declare(x) \
    void p##x(const char *msg, ...); \
    void ph##x(const char *header, const char *msg, ...);

declare(red)
declare(blue)
declare(grn)
declare(ylw)
declare(mgn)
declare(cyn)

#undef declare

#ifdef DEBUG
#define pdbg(x, ...) ddbg( ANSI_FONT_BOLD "<%s:%d:%s> " ANSI_COLOR_RESET x, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
void ddbg(const char *msg, ...);

#define perr(x, ...) derr( ANSI_FONT_BOLD "<%s:%d:%s> " ANSI_COLOR_RESET x, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
void derr(const char *msg, ...);

#define pinfo(x, ...) dinfo( ANSI_FONT_BOLD "<%s:%d:%s> " ANSI_COLOR_RESET x, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
void dinfo(const char *msg, ...);

#define pwarn(x, ...) dwarn( ANSI_FONT_BOLD "<%s:%d:%s> " ANSI_COLOR_RESET x, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
void dwarn(const char *msg, ...);
#else
void pdbg(const char *msg, ...);
void perr(const char *msg, ...);
void pinfo(const char *msg, ...);
void pwarn(const char *msg, ...);
#endif
