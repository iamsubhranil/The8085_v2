#pragma once

#include "display.h"

// Highlight a keyword in entered state
#define hkw(x) ANSI_COLOR_GREEN #x ANSI_COLOR_RESET
// Highlight an instruction
#define hins(x) ANSI_FONT_BOLD #x ANSI_COLOR_RESET
// The prompt symbol
#define hprmpt ANSI_FONT_BOLD " >> " ANSI_COLOR_RESET
// A code example
#define hcode(x) "      " hprmpt hkw(x) " "
// A 'Usage' section
#define husage(x) "Usage : \n" hcode(x)
