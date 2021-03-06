#pragma once

#include "scanner.h"

// Interface for code generator
// ============================
// The compiler only calls these functions
// to maintain a level of abstraction
// with the code generator.

void codegen_no_op(Token op);
void codegen_reg(Token op, Token reg);
void codegen_mem(Token op);
void codegen_regpair(Token op, Token regp);
void codegen_sp(Token op);
void codegen_psw(Token op);
void codegen_mov_r_r(Token dest, Token source);
void codegen_mov_r_m(Token dest);
void codegen_mov_m_r(Token source);
