#include "bytecode.h"

#ifdef USE_NEOVM

#include "display.h"
#include <stdio.h>

static u16 intrpointer = 0;

void bytecode_disassemble_in_context(u8 *memory, u16 pointer, Machine *m){
    (void)m;
    bytecode_disassemble(memory, pointer);
}

void bytecode_disassemble_chunk(u8 *memory, u16 pointer, u16 upto){
    intrpointer = pointer;
    while(intrpointer <= upto){
        bytecode_disassemble(memory, intrpointer);
    }
}

void bytecode_disassemble(u8 *memory, u16 pointer){
    intrpointer = pointer + 1;
    u8 opcode = memory[pointer];
    pred("\n%04x: ", pointer);
    #define DECODE_BYTE()   pylw("   %02xh", memory[intrpointer++]);
    #define DECODE_WORD()   pylw(" %02x%02xh", memory[intrpointer + 1], memory[intrpointer]); intrpointer += 2;
    switch(opcode){ 
        case 0xCE: 
			pblue("%5s\t", "aci");
            DECODE_BYTE();
            break;
        case 0x8F: 
			pblue("%5s\t", "adc");
			pgrn("%6s", "a"); 
            break;
        case 0x88: 
			pblue("%5s\t", "adc");
			pgrn("%6s", "b"); 
            break;
        case 0x89: 
			pblue("%5s\t", "adc");
			pgrn("%6s", "c"); 
            break;
        case 0x8A: 
			pblue("%5s\t", "adc");
			pgrn("%6s", "d"); 
            break;
        case 0x8B: 
			pblue("%5s\t", "adc");
			pgrn("%6s", "e"); 
            break;
        case 0x8C: 
			pblue("%5s\t", "adc");
			pgrn("%6s", "h"); 
            break;
        case 0x8D: 
			pblue("%5s\t", "adc");
			pgrn("%6s", "l"); 
            break;
        case 0x8E: 
			pblue("%5s\t", "adc");
			pgrn("%6s", "m"); 
            break;
        case 0x87: 
			pblue("%5s\t", "add");
			pgrn("%6s", "a"); 
            break;
        case 0x80: 
			pblue("%5s\t", "add");
			pgrn("%6s", "b"); 
            break;
        case 0x81: 
			pblue("%5s\t", "add");
			pgrn("%6s", "c"); 
            break;
        case 0x82: 
			pblue("%5s\t", "add");
			pgrn("%6s", "d"); 
            break;
        case 0x83: 
			pblue("%5s\t", "add");
			pgrn("%6s", "e"); 
            break;
        case 0x84: 
			pblue("%5s\t", "add");
			pgrn("%6s", "h"); 
            break;
        case 0x85: 
			pblue("%5s\t", "add");
			pgrn("%6s", "l"); 
            break;
        case 0x86: 
			pblue("%5s\t", "add");
			pgrn("%6s", "m"); 
            break;
        case 0xC6: 
			pblue("%5s\t", "adi");
            DECODE_BYTE();
            break;
        case 0xA7: 
			pblue("%5s\t", "ana");
			pgrn("%6s", "a"); 
            break;
        case 0xA0: 
			pblue("%5s\t", "ana");
			pgrn("%6s", "b"); 
            break;
        case 0xA1: 
			pblue("%5s\t", "ana");
			pgrn("%6s", "c"); 
            break;
        case 0xA2: 
			pblue("%5s\t", "ana");
			pgrn("%6s", "d"); 
            break;
        case 0xA3: 
			pblue("%5s\t", "ana");
			pgrn("%6s", "e"); 
            break;
        case 0xA4: 
			pblue("%5s\t", "ana");
			pgrn("%6s", "h"); 
            break;
        case 0xA5: 
			pblue("%5s\t", "ana");
			pgrn("%6s", "l"); 
            break;
        case 0xA6: 
			pblue("%5s\t", "ana");
			pgrn("%6s", "m"); 
            break;
        case 0xE6: 
			pblue("%5s\t", "ani");
            DECODE_BYTE();
            break;
        case 0xCD: 
			pblue("%5s\t", "call");
            DECODE_WORD();
            break;
        case 0xDC: 
			pblue("%5s\t", "cc");
            DECODE_WORD();
            break;
        case 0xFC: 
			pblue("%5s\t", "cm");
            DECODE_WORD();
            break;
        case 0x2F: 
			pblue("%5s\t", "cma");
            break;
        case 0x3F: 
			pblue("%5s\t", "cmc");
            break;
        case 0xBF: 
			pblue("%5s\t", "cmp");
			pgrn("%6s", "a"); 
            break;
        case 0xB8: 
			pblue("%5s\t", "cmp");
			pgrn("%6s", "b"); 
            break;
        case 0xB9: 
			pblue("%5s\t", "cmp");
			pgrn("%6s", "c"); 
            break;
        case 0xBA: 
			pblue("%5s\t", "cmp");
			pgrn("%6s", "d"); 
            break;
        case 0xBB: 
			pblue("%5s\t", "cmp");
			pgrn("%6s", "e"); 
            break;
        case 0xBC: 
			pblue("%5s\t", "cmp");
			pgrn("%6s", "h"); 
            break;
        case 0xBD: 
			pblue("%5s\t", "cmp");
			pgrn("%6s", "l"); 
            break;
        case 0xBE: 
			pblue("%5s\t", "cmp");
			pgrn("%6s", "m"); 
            break;
        case 0xD4: 
			pblue("%5s\t", "cnc");
            DECODE_WORD();
            break;
        case 0xC4: 
			pblue("%5s\t", "cnz");
            DECODE_WORD();
            break;
        case 0xF4: 
			pblue("%5s\t", "cp");
            DECODE_WORD();
            break;
        case 0xEC: 
			pblue("%5s\t", "cpe");
            DECODE_WORD();
            break;
        case 0xFE: 
			pblue("%5s\t", "cpi");
            DECODE_BYTE();
            break;
        case 0xE4: 
			pblue("%5s\t", "cpo");
            DECODE_WORD();
            break;
        case 0xCC: 
			pblue("%5s\t", "cz");
            DECODE_WORD();
            break;
        case 0x27 : 
			pblue("%5s\t", "daa");
            break;
        case 0x09: 
			pblue("%5s\t", "dad");
			pgrn("%6s", "b"); 
            break;
        case 0x19: 
			pblue("%5s\t", "dad");
			pgrn("%6s", "d"); 
            break;
        case 0x29: 
			pblue("%5s\t", "dad");
			pgrn("%6s", "h"); 
            break;
        case 0x39: 
			pblue("%5s\t", "dad");
			pgrn("%6s", "sp"); 
            break;
        case 0x3D: 
			pblue("%5s\t", "dcr");
			pgrn("%6s", "a"); 
            break;
        case 0x05: 
			pblue("%5s\t", "dcr");
			pgrn("%6s", "b"); 
            break;
        case 0x0D: 
			pblue("%5s\t", "dcr");
			pgrn("%6s", "c"); 
            break;
        case 0x15: 
			pblue("%5s\t", "dcr");
			pgrn("%6s", "d"); 
            break;
        case 0x1D: 
			pblue("%5s\t", "dcr");
			pgrn("%6s", "e"); 
            break;
        case 0x25: 
			pblue("%5s\t", "dcr");
			pgrn("%6s", "h"); 
            break;
        case 0x2D: 
			pblue("%5s\t", "dcr");
			pgrn("%6s", "l"); 
            break;
        case 0x35: 
			pblue("%5s\t", "dcr");
			pgrn("%6s", "m"); 
            break;
        case 0x0B: 
			pblue("%5s\t", "dcx");
			pgrn("%6s", "b"); 
            break;
        case 0x1B: 
			pblue("%5s\t", "dcx");
			pgrn("%6s", "d"); 
            break;
        case 0x2B: 
			pblue("%5s\t", "dcx");
			pgrn("%6s", "h"); 
            break;
        case 0x3B: 
			pblue("%5s\t", "dcx");
			pgrn("%6s", "sp"); 
            break;
        case 0xF3: 
			pblue("%5s\t", "di");
            break;
        case 0xFB: 
			pblue("%5s\t", "ei");
            break;
        case 0x76: 
			pblue("%5s\t", "hlt");
            break;
        case 0xDB: 
			pblue("%5s\t", "in");
            DECODE_BYTE();
            break;
        case 0x3C: 
			pblue("%5s\t", "inr");
			pgrn("%6s", "a"); 
            break;
        case 0x04: 
			pblue("%5s\t", "inr");
			pgrn("%6s", "b"); 
            break;
        case 0x0C: 
			pblue("%5s\t", "inr");
			pgrn("%6s", "c"); 
            break;
        case 0x14: 
			pblue("%5s\t", "inr");
			pgrn("%6s", "d"); 
            break;
        case 0x1C: 
			pblue("%5s\t", "inr");
			pgrn("%6s", "e"); 
            break;
        case 0x24: 
			pblue("%5s\t", "inr");
			pgrn("%6s", "h"); 
            break;
        case 0x2C: 
			pblue("%5s\t", "inr");
			pgrn("%6s", "l"); 
            break;
        case 0x34: 
			pblue("%5s\t", "inr");
			pgrn("%6s", "m"); 
            break;
        case 0x03: 
			pblue("%5s\t", "inx");
			pgrn("%6s", "b"); 
            break;
        case 0x13: 
			pblue("%5s\t", "inx");
			pgrn("%6s", "d"); 
            break;
        case 0x23: 
			pblue("%5s\t", "inx");
			pgrn("%6s", "h"); 
            break;
        case 0x33: 
			pblue("%5s\t", "inx");
			pgrn("%6s", "sp"); 
            break;
        case 0xDA: 
			pblue("%5s\t", "jc");
            DECODE_WORD();
            break;
        case 0xFA: 
			pblue("%5s\t", "jm");
            DECODE_WORD();
            break;
        case 0xC3: 
			pblue("%5s\t", "jmp");
            DECODE_WORD();
            break;
        case 0xD2: 
			pblue("%5s\t", "jnc");
            DECODE_WORD();
            break;
        case 0xC2: 
			pblue("%5s\t", "jnz");
            DECODE_WORD();
            break;
        case 0xF2: 
			pblue("%5s\t", "jp");
            DECODE_WORD();
            break;
        case 0xEA: 
			pblue("%5s\t", "jpe");
            DECODE_WORD();
            break;
        case 0xE2: 
			pblue("%5s\t", "jpo");
            DECODE_WORD();
            break;
        case 0xCA: 
			pblue("%5s\t", "jz");
            DECODE_WORD();
            break;
        case 0x3A: 
			pblue("%5s\t", "lda");
            DECODE_WORD();
            break;
        case 0x0A: 
			pblue("%5s\t", "ldax");
			pgrn("%6s", "b"); 
            break;
        case 0x1A: 
			pblue("%5s\t", "ldax");
			pgrn("%6s", "d"); 
            break;
        case 0x2A: 
			pblue("%5s\t", "lhld");
            DECODE_WORD();
            break;
        case 0x01: 
			pblue("%5s\t", "lxi");
			pgrn("%6s", "b");
			printf(",");
            DECODE_WORD();
            break;
        case 0x11: 
			pblue("%5s\t", "lxi");
			pgrn("%6s", "d");
			printf(",");
            DECODE_WORD();
            break;
        case 0x21: 
			pblue("%5s\t", "lxi");
			pgrn("%6s", "h");
			printf(",");
            DECODE_WORD();
            break;
        case 0x31: 
			pblue("%5s\t", "lxi");
			pgrn("%6s", "sp");
			printf(",");
            DECODE_WORD();
            break;
        case 0x7F: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "a");
			printf(",");
			pgrn("%6s", "a"); 
            break;
        case 0x78: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "a");
			printf(",");
			pgrn("%6s", "b"); 
            break;
        case 0x79: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "a");
			printf(",");
			pgrn("%6s", "c"); 
            break;
        case 0x7A: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "a");
			printf(",");
			pgrn("%6s", "d"); 
            break;
        case 0x7B: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "a");
			printf(",");
			pgrn("%6s", "e"); 
            break;
        case 0x7C: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "a");
			printf(",");
			pgrn("%6s", "h"); 
            break;
        case 0x7D: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "a");
			printf(",");
			pgrn("%6s", "l"); 
            break;
        case 0x7E: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "a");
			printf(",");
			pgrn("%6s", "m"); 
            break;
        case 0x47: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "b");
			printf(",");
			pgrn("%6s", "a"); 
            break;
        case 0x40: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "b");
			printf(",");
			pgrn("%6s", "b"); 
            break;
        case 0x41: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "b");
			printf(",");
			pgrn("%6s", "c"); 
            break;
        case 0x42: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "b");
			printf(",");
			pgrn("%6s", "d"); 
            break;
        case 0x43: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "b");
			printf(",");
			pgrn("%6s", "e"); 
            break;
        case 0x44: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "b");
			printf(",");
			pgrn("%6s", "h"); 
            break;
        case 0x45: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "b");
			printf(",");
			pgrn("%6s", "l"); 
            break;
        case 0x46: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "b");
			printf(",");
			pgrn("%6s", "m"); 
            break;
        case 0x4F: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "c");
			printf(",");
			pgrn("%6s", "a"); 
            break;
        case 0x48: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "c");
			printf(",");
			pgrn("%6s", "b"); 
            break;
        case 0x49: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "c");
			printf(",");
			pgrn("%6s", "c"); 
            break;
        case 0x4A: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "c");
			printf(",");
			pgrn("%6s", "d"); 
            break;
        case 0x4B: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "c");
			printf(",");
			pgrn("%6s", "e"); 
            break;
        case 0x4C: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "c");
			printf(",");
			pgrn("%6s", "h"); 
            break;
        case 0x4D: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "c");
			printf(",");
			pgrn("%6s", "l"); 
            break;
        case 0x4E: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "c");
			printf(",");
			pgrn("%6s", "m"); 
            break;
        case 0x57: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "d");
			printf(",");
			pgrn("%6s", "a"); 
            break;
        case 0x50: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "d");
			printf(",");
			pgrn("%6s", "b"); 
            break;
        case 0x51: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "d");
			printf(",");
			pgrn("%6s", "c"); 
            break;
        case 0x52: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "d");
			printf(",");
			pgrn("%6s", "d"); 
            break;
        case 0x53: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "d");
			printf(",");
			pgrn("%6s", "e"); 
            break;
        case 0x54: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "d");
			printf(",");
			pgrn("%6s", "h"); 
            break;
        case 0x55: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "d");
			printf(",");
			pgrn("%6s", "l"); 
            break;
        case 0x56: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "d");
			printf(",");
			pgrn("%6s", "m"); 
            break;
        case 0x5F: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "e");
			printf(",");
			pgrn("%6s", "a"); 
            break;
        case 0x58: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "e");
			printf(",");
			pgrn("%6s", "b"); 
            break;
        case 0x59: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "e");
			printf(",");
			pgrn("%6s", "c"); 
            break;
        case 0x5A: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "e");
			printf(",");
			pgrn("%6s", "d"); 
            break;
        case 0x5B: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "e");
			printf(",");
			pgrn("%6s", "e"); 
            break;
        case 0x5C: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "e");
			printf(",");
			pgrn("%6s", "h"); 
            break;
        case 0x5D: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "e");
			printf(",");
			pgrn("%6s", "l"); 
            break;
        case 0x5E: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "e");
			printf(",");
			pgrn("%6s", "m"); 
            break;
        case 0x67: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "h");
			printf(",");
			pgrn("%6s", "a"); 
            break;
        case 0x60: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "h");
			printf(",");
			pgrn("%6s", "b"); 
            break;
        case 0x61: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "h");
			printf(",");
			pgrn("%6s", "c"); 
            break;
        case 0x62: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "h");
			printf(",");
			pgrn("%6s", "d"); 
            break;
        case 0x63: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "h");
			printf(",");
			pgrn("%6s", "e"); 
            break;
        case 0x64: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "h");
			printf(",");
			pgrn("%6s", "h"); 
            break;
        case 0x65: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "h");
			printf(",");
			pgrn("%6s", "l"); 
            break;
        case 0x66: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "h");
			printf(",");
			pgrn("%6s", "m"); 
            break;
        case 0x6F: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "l");
			printf(",");
			pgrn("%6s", "a"); 
            break;
        case 0x68: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "l");
			printf(",");
			pgrn("%6s", "b"); 
            break;
        case 0x69: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "l");
			printf(",");
			pgrn("%6s", "c"); 
            break;
        case 0x6A: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "l");
			printf(",");
			pgrn("%6s", "d"); 
            break;
        case 0x6B: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "l");
			printf(",");
			pgrn("%6s", "e"); 
            break;
        case 0x6C: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "l");
			printf(",");
			pgrn("%6s", "h"); 
            break;
        case 0x6D: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "l");
			printf(",");
			pgrn("%6s", "l"); 
            break;
        case 0x6E: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "l");
			printf(",");
			pgrn("%6s", "m"); 
            break;
        case 0x77: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "m");
			printf(",");
			pgrn("%6s", "a"); 
            break;
        case 0x70: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "m");
			printf(",");
			pgrn("%6s", "b"); 
            break;
        case 0x71: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "m");
			printf(",");
			pgrn("%6s", "c"); 
            break;
        case 0x72: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "m");
			printf(",");
			pgrn("%6s", "d"); 
            break;
        case 0x73: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "m");
			printf(",");
			pgrn("%6s", "e"); 
            break;
        case 0x74: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "m");
			printf(",");
			pgrn("%6s", "h"); 
            break;
        case 0x75: 
			pblue("%5s\t", "mov");
			pgrn("%6s", "m");
			printf(",");
			pgrn("%6s", "l"); 
            break;
        case 0x3E: 
			pblue("%5s\t", "mvi");
			pgrn("%6s", "a");
			printf(",");
            DECODE_BYTE();
            break;
        case 0x06: 
			pblue("%5s\t", "mvi");
			pgrn("%6s", "b");
			printf(",");
            DECODE_BYTE();
            break;
        case 0x0E: 
			pblue("%5s\t", "mvi");
			pgrn("%6s", "c");
			printf(",");
            DECODE_BYTE();
            break;
        case 0x16: 
			pblue("%5s\t", "mvi");
			pgrn("%6s", "d");
			printf(",");
            DECODE_BYTE();
            break;
        case 0x1E: 
			pblue("%5s\t", "mvi");
			pgrn("%6s", "e");
			printf(",");
            DECODE_BYTE();
            break;
        case 0x26: 
			pblue("%5s\t", "mvi");
			pgrn("%6s", "h");
			printf(",");
            DECODE_BYTE();
            break;
        case 0x2E: 
			pblue("%5s\t", "mvi");
			pgrn("%6s", "l");
			printf(",");
            DECODE_BYTE();
            break;
        case 0x36: 
			pblue("%5s\t", "mvi");
			pgrn("%6s", "m");
			printf(",");
            DECODE_BYTE();
            break;
        case 0x00: 
			pblue("%5s\t", "nop");
            break;
        case 0xB7: 
			pblue("%5s\t", "ora");
			pgrn("%6s", "a"); 
            break;
        case 0xB0: 
			pblue("%5s\t", "ora");
			pgrn("%6s", "b"); 
            break;
        case 0xB1: 
			pblue("%5s\t", "ora");
			pgrn("%6s", "c"); 
            break;
        case 0xB2: 
			pblue("%5s\t", "ora");
			pgrn("%6s", "d"); 
            break;
        case 0xB3: 
			pblue("%5s\t", "ora");
			pgrn("%6s", "e"); 
            break;
        case 0xB4: 
			pblue("%5s\t", "ora");
			pgrn("%6s", "h"); 
            break;
        case 0xB5: 
			pblue("%5s\t", "ora");
			pgrn("%6s", "l"); 
            break;
        case 0xB6: 
			pblue("%5s\t", "ora");
			pgrn("%6s", "m"); 
            break;
        case 0xF6: 
			pblue("%5s\t", "ori");
            DECODE_BYTE();
            break;
        case 0xD3: 
			pblue("%5s\t", "out");
            DECODE_BYTE();
            break;
        case 0xE9: 
			pblue("%5s\t", "pchl");
            break;
        case 0xC1: 
			pblue("%5s\t", "pop");
			pgrn("%6s", "b"); 
            break;
        case 0xD1: 
			pblue("%5s\t", "pop");
			pgrn("%6s", "d"); 
            break;
        case 0xE1: 
			pblue("%5s\t", "pop");
			pgrn("%6s", "h"); 
            break;
        case 0xF1: 
			pblue("%5s\t", "pop");
			pgrn("%6s", "psw"); 
            break;
        case 0xC5: 
			pblue("%5s\t", "push");
			pgrn("%6s", "b"); 
            break;
        case 0xD5: 
			pblue("%5s\t", "push");
			pgrn("%6s", "d"); 
            break;
        case 0xE5: 
			pblue("%5s\t", "push");
			pgrn("%6s", "h"); 
            break;
        case 0xF5: 
			pblue("%5s\t", "push");
			pgrn("%6s", "psw"); 
            break;
        case 0x17: 
			pblue("%5s\t", "ral");
            break;
        case 0x1F: 
			pblue("%5s\t", "rar");
            break;
        case 0xD8: 
			pblue("%5s\t", "rc");
            break;
        case 0xC9: 
			pblue("%5s\t", "ret");
            break;
        case 0x20: 
			pblue("%5s\t", "rim");
            break;
        case 0x07: 
			pblue("%5s\t", "rlc");
            break;
        case 0xF8: 
			pblue("%5s\t", "rm");
            break;
        case 0xD0: 
			pblue("%5s\t", "rnc");
            break;
        case 0xC0: 
			pblue("%5s\t", "rnz");
            break;
        case 0xF0: 
			pblue("%5s\t", "rp");
            break;
        case 0xE8: 
			pblue("%5s\t", "rpe");
            break;
        case 0xE0: 
			pblue("%5s\t", "rpo");
            break;
        case 0x0F: 
			pblue("%5s\t", "rrc");
            break;
        case 0xC7: 
			pblue("%5s\t", "rst");
			pgrn("%6s", "0"); 
            break;
        case 0xCF: 
			pblue("%5s\t", "rst");
			pgrn("%6s", "1"); 
            break;
        case 0xD7: 
			pblue("%5s\t", "rst");
			pgrn("%6s", "2"); 
            break;
        case 0xDF: 
			pblue("%5s\t", "rst");
			pgrn("%6s", "3"); 
            break;
        case 0xE7: 
			pblue("%5s\t", "rst");
			pgrn("%6s", "4"); 
            break;
        case 0xEF: 
			pblue("%5s\t", "rst");
			pgrn("%6s", "5"); 
            break;
        case 0xF7: 
			pblue("%5s\t", "rst");
			pgrn("%6s", "6"); 
            break;
        case 0xFF: 
			pblue("%5s\t", "rst");
			pgrn("%6s", "7"); 
            break;
        case 0xC8: 
			pblue("%5s\t", "rz");
            break;
        case 0x9F: 
			pblue("%5s\t", "sbb");
			pgrn("%6s", "a"); 
            break;
        case 0x98: 
			pblue("%5s\t", "sbb");
			pgrn("%6s", "b"); 
            break;
        case 0x99: 
			pblue("%5s\t", "sbb");
			pgrn("%6s", "c"); 
            break;
        case 0x9A: 
			pblue("%5s\t", "sbb");
			pgrn("%6s", "d"); 
            break;
        case 0x9B: 
			pblue("%5s\t", "sbb");
			pgrn("%6s", "e"); 
            break;
        case 0x9C: 
			pblue("%5s\t", "sbb");
			pgrn("%6s", "h"); 
            break;
        case 0x9D: 
			pblue("%5s\t", "sbb");
			pgrn("%6s", "l"); 
            break;
        case 0x9E: 
			pblue("%5s\t", "sbb");
			pgrn("%6s", "m"); 
            break;
        case 0xDE: 
			pblue("%5s\t", "sbi");
            DECODE_BYTE();
            break;
        case 0x22: 
			pblue("%5s\t", "shld");
            DECODE_WORD();
            break;
        case 0x30: 
			pblue("%5s\t", "sim");
            break;
        case 0xF9: 
			pblue("%5s\t", "sphl");
            break;
        case 0x32: 
			pblue("%5s\t", "sta");
            DECODE_WORD();
            break;
        case 0x02: 
			pblue("%5s\t", "stax");
			pgrn("%6s", "b"); 
            break;
        case 0x12: 
			pblue("%5s\t", "stax");
			pgrn("%6s", "d"); 
            break;
        case 0x37: 
			pblue("%5s\t", "stc");
            break;
        case 0x97: 
			pblue("%5s\t", "sub");
			pgrn("%6s", "a"); 
            break;
        case 0x90: 
			pblue("%5s\t", "sub");
			pgrn("%6s", "b"); 
            break;
        case 0x91: 
			pblue("%5s\t", "sub");
			pgrn("%6s", "c"); 
            break;
        case 0x92: 
			pblue("%5s\t", "sub");
			pgrn("%6s", "d"); 
            break;
        case 0x93: 
			pblue("%5s\t", "sub");
			pgrn("%6s", "e"); 
            break;
        case 0x94: 
			pblue("%5s\t", "sub");
			pgrn("%6s", "h"); 
            break;
        case 0x95: 
			pblue("%5s\t", "sub");
			pgrn("%6s", "l"); 
            break;
        case 0x96: 
			pblue("%5s\t", "sub");
			pgrn("%6s", "m"); 
            break;
        case 0xD6: 
			pblue("%5s\t", "sui");
            DECODE_BYTE();
            break;
        case 0xEB: 
			pblue("%5s\t", "xchg");
            break;
        case 0xAF: 
			pblue("%5s\t", "xra");
			pgrn("%6s", "a"); 
            break;
        case 0xA8: 
			pblue("%5s\t", "xra");
			pgrn("%6s", "b"); 
            break;
        case 0xA9: 
			pblue("%5s\t", "xra");
			pgrn("%6s", "c"); 
            break;
        case 0xAA: 
			pblue("%5s\t", "xra");
			pgrn("%6s", "d"); 
            break;
        case 0xAB: 
			pblue("%5s\t", "xra");
			pgrn("%6s", "e"); 
            break;
        case 0xAC: 
			pblue("%5s\t", "xra");
			pgrn("%6s", "h"); 
            break;
        case 0xAD: 
			pblue("%5s\t", "xra");
			pgrn("%6s", "l"); 
            break;
        case 0xAE: 
			pblue("%5s\t", "xra");
			pgrn("%6s", "m"); 
            break;
        case 0xEE: 
			pblue("%5s\t", "xri");
            DECODE_BYTE();
            break;
        case 0xE3: 
			pblue("%5s\t", "xthl");
            break;
    }
}

#endif
