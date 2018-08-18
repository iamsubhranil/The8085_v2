#include "bytecode.h"

#ifdef USE_NEOVM

#include "display.h"

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
    pred("\n%x: ", pointer);
    #define DECODE_BYTE()   pblue("%xh", memory[intrpointer++]);
    #define DECODE_WORD()   pblue("%x%xh", memory[intrpointer + 1], memory[intrpointer]); intrpointer += 2;
    switch(opcode){
        case 0xCE: 
            pylw("aci ");
            DECODE_BYTE();
            break;
        case 0x8F: 
            pylw("adc a");
            break;
        case 0x88: 
            pylw("adc b");
            break;
        case 0x89: 
            pylw("adc c");
            break;
        case 0x8A: 
            pylw("adc d");
            break;
        case 0x8B: 
            pylw("adc e");
            break;
        case 0x8C: 
            pylw("adc h");
            break;
        case 0x8D: 
            pylw("adc l");
            break;
        case 0x8E: 
            pylw("adc m");
            break;
        case 0x87: 
            pylw("add a");
            break;
        case 0x80: 
            pylw("add b");
            break;
        case 0x81: 
            pylw("add c");
            break;
        case 0x82: 
            pylw("add d");
            break;
        case 0x83: 
            pylw("add e");
            break;
        case 0x84: 
            pylw("add h");
            break;
        case 0x85: 
            pylw("add l");
            break;
        case 0x86: 
            pylw("add m");
            break;
        case 0xC6: 
            pylw("adi ");
            DECODE_BYTE();
            break;
        case 0xA7: 
            pylw("ana a");
            break;
        case 0xA0: 
            pylw("ana b");
            break;
        case 0xA1: 
            pylw("ana c");
            break;
        case 0xA2: 
            pylw("ana d");
            break;
        case 0xA3: 
            pylw("ana e");
            break;
        case 0xA4: 
            pylw("ana h");
            break;
        case 0xA5: 
            pylw("ana l");
            break;
        case 0xA6: 
            pylw("ana m");
            break;
        case 0xE6: 
            pylw("ani ");
            DECODE_BYTE();
            break;
        case 0xCD: 
            pylw("call ");
            DECODE_WORD();
            break;
        case 0xDC: 
            pylw("cc ");
            DECODE_WORD();
            break;
        case 0xFC: 
            pylw("cm ");
            DECODE_WORD();
            break;
        case 0x2F: 
            pylw("cma");
            break;
        case 0x3F: 
            pylw("cmc");
            break;
        case 0xBF: 
            pylw("cmp a");
            break;
        case 0xB8: 
            pylw("cmp b");
            break;
        case 0xB9: 
            pylw("cmp c");
            break;
        case 0xBA: 
            pylw("cmp d");
            break;
        case 0xBB: 
            pylw("cmp e");
            break;
        case 0xBC: 
            pylw("cmp h");
            break;
        case 0xBD: 
            pylw("cmp l");
            break;
        case 0xBE: 
            pylw("cmp m");
            break;
        case 0xD4: 
            pylw("cnc ");
            DECODE_WORD();
            break;
        case 0xC4: 
            pylw("cnz ");
            DECODE_WORD();
            break;
        case 0xF4: 
            pylw("cp ");
            DECODE_WORD();
            break;
        case 0xEC: 
            pylw("cpe ");
            DECODE_WORD();
            break;
        case 0xFE: 
            pylw("cpi ");
            DECODE_BYTE();
            break;
        case 0xE4: 
            pylw("cpo ");
            DECODE_WORD();
            break;
        case 0xCC: 
            pylw("cz ");
            DECODE_WORD();
            break;
        case 0x27 : 
            pylw("daa");
            break;
        case 0x09: 
            pylw("dad b");
            break;
        case 0x19: 
            pylw("dad d");
            break;
        case 0x29: 
            pylw("dad h");
            break;
        case 0x39: 
            pylw("dad sp");
            break;
        case 0x3D: 
            pylw("dcr a");
            break;
        case 0x05: 
            pylw("dcr b");
            break;
        case 0x0D: 
            pylw("dcr c");
            break;
        case 0x15: 
            pylw("dcr d");
            break;
        case 0x1D: 
            pylw("dcr e");
            break;
        case 0x25: 
            pylw("dcr h");
            break;
        case 0x2D: 
            pylw("dcr l");
            break;
        case 0x35: 
            pylw("dcr m");
            break;
        case 0x0B: 
            pylw("dcx b");
            break;
        case 0x1B: 
            pylw("dcx d");
            break;
        case 0x2B: 
            pylw("dcx h");
            break;
        case 0x3B: 
            pylw("dcx sp");
            break;
        case 0xF3: 
            pylw("di");
            break;
        case 0xFB: 
            pylw("ei");
            break;
        case 0x76: 
            pylw("hlt");
            break;
        case 0xDB: 
            pylw("in ");
            DECODE_BYTE();
            break;
        case 0x3C: 
            pylw("inr a");
            break;
        case 0x04: 
            pylw("inr b");
            break;
        case 0x0C: 
            pylw("inr c");
            break;
        case 0x14: 
            pylw("inr d");
            break;
        case 0x1C: 
            pylw("inr e");
            break;
        case 0x24: 
            pylw("inr h");
            break;
        case 0x2C: 
            pylw("inr l");
            break;
        case 0x34: 
            pylw("inr m");
            break;
        case 0x03: 
            pylw("inx b");
            break;
        case 0x13: 
            pylw("inx d");
            break;
        case 0x23: 
            pylw("inx h");
            break;
        case 0x33: 
            pylw("inx sp");
            break;
        case 0xDA: 
            pylw("jc ");
            DECODE_WORD();
            break;
        case 0xFA: 
            pylw("jm ");
            DECODE_WORD();
            break;
        case 0xC3: 
            pylw("jmp ");
            DECODE_WORD();
            break;
        case 0xD2: 
            pylw("jnc ");
            DECODE_WORD();
            break;
        case 0xC2: 
            pylw("jnz ");
            DECODE_WORD();
            break;
        case 0xF2: 
            pylw("jp ");
            DECODE_WORD();
            break;
        case 0xEA: 
            pylw("jpe ");
            DECODE_WORD();
            break;
        case 0xE2: 
            pylw("jpo ");
            DECODE_WORD();
            break;
        case 0xCA: 
            pylw("jz ");
            DECODE_WORD();
            break;
        case 0x3A: 
            pylw("lda ");
            DECODE_WORD();
            break;
        case 0x0A: 
            pylw("ldax b");
            break;
        case 0x1A: 
            pylw("ldax d");
            break;
        case 0x2A: 
            pylw("lhld ");
            DECODE_WORD();
            break;
        case 0x01: 
            pylw("lxi b, ");
            DECODE_WORD();
            break;
        case 0x11: 
            pylw("lxi d, ");
            DECODE_WORD();
            break;
        case 0x21: 
            pylw("lxi h, ");
            DECODE_WORD();
            break;
        case 0x31: 
            pylw("lxi sp, ");
            DECODE_WORD();
            break;
        case 0x7F: 
            pylw("mov a, a");
            break;
        case 0x78: 
            pylw("mov a, b");
            break;
        case 0x79: 
            pylw("mov a, c");
            break;
        case 0x7A: 
            pylw("mov a, d");
            break;
        case 0x7B: 
            pylw("mov a, e");
            break;
        case 0x7C: 
            pylw("mov a, h");
            break;
        case 0x7D: 
            pylw("mov a, l");
            break;
        case 0x7E: 
            pylw("mov a, m");
            break;
        case 0x47: 
            pylw("mov b, a");
            break;
        case 0x40: 
            pylw("mov b, b");
            break;
        case 0x41: 
            pylw("mov b, c");
            break;
        case 0x42: 
            pylw("mov b, d");
            break;
        case 0x43: 
            pylw("mov b, e");
            break;
        case 0x44: 
            pylw("mov b, h");
            break;
        case 0x45: 
            pylw("mov b, l");
            break;
        case 0x46: 
            pylw("mov b, m");
            break;
        case 0x4F: 
            pylw("mov c, a");
            break;
        case 0x48: 
            pylw("mov c, b");
            break;
        case 0x49: 
            pylw("mov c, c");
            break;
        case 0x4A: 
            pylw("mov c, d");
            break;
        case 0x4B: 
            pylw("mov c, e");
            break;
        case 0x4C: 
            pylw("mov c, h");
            break;
        case 0x4D: 
            pylw("mov c, l");
            break;
        case 0x4E: 
            pylw("mov c, m");
            break;
        case 0x57: 
            pylw("mov d, a");
            break;
        case 0x50: 
            pylw("mov d, b");
            break;
        case 0x51: 
            pylw("mov d, c");
            break;
        case 0x52: 
            pylw("mov d, d");
            break;
        case 0x53: 
            pylw("mov d, e");
            break;
        case 0x54: 
            pylw("mov d, h");
            break;
        case 0x55: 
            pylw("mov d, l");
            break;
        case 0x56: 
            pylw("mov d, m");
            break;
        case 0x5F: 
            pylw("mov e, a");
            break;
        case 0x58: 
            pylw("mov e, b");
            break;
        case 0x59: 
            pylw("mov e, c");
            break;
        case 0x5A: 
            pylw("mov e, d");
            break;
        case 0x5B: 
            pylw("mov e, e");
            break;
        case 0x5C: 
            pylw("mov e, h");
            break;
        case 0x5D: 
            pylw("mov e, l");
            break;
        case 0x5E: 
            pylw("mov e, m");
            break;
        case 0x67: 
            pylw("mov h, a");
            break;
        case 0x60: 
            pylw("mov h, b");
            break;
        case 0x61: 
            pylw("mov h, c");
            break;
        case 0x62: 
            pylw("mov h, d");
            break;
        case 0x63: 
            pylw("mov h, e");
            break;
        case 0x64: 
            pylw("mov h, h");
            break;
        case 0x65: 
            pylw("mov h, l");
            break;
        case 0x66: 
            pylw("mov h, m");
            break;
        case 0x6F: 
            pylw("mov l, a");
            break;
        case 0x68: 
            pylw("mov l, b");
            break;
        case 0x69: 
            pylw("mov l, c");
            break;
        case 0x6A: 
            pylw("mov l, d");
            break;
        case 0x6B: 
            pylw("mov l, e");
            break;
        case 0x6C: 
            pylw("mov l, h");
            break;
        case 0x6D: 
            pylw("mov l, l");
            break;
        case 0x6E: 
            pylw("mov l, m");
            break;
        case 0x77: 
            pylw("mov m, a");
            break;
        case 0x70: 
            pylw("mov m, b");
            break;
        case 0x71: 
            pylw("mov m, c");
            break;
        case 0x72: 
            pylw("mov m, d");
            break;
        case 0x73: 
            pylw("mov m, e");
            break;
        case 0x74: 
            pylw("mov m, h");
            break;
        case 0x75: 
            pylw("mov m, l");
            break;
        case 0x3E: 
            pylw("mvi a, ");
            DECODE_BYTE();
            break;
        case 0x06: 
            pylw("mvi b, ");
            DECODE_BYTE();
            break;
        case 0x0E: 
            pylw("mvi c, ");
            DECODE_BYTE();
            break;
        case 0x16: 
            pylw("mvi d, ");
            DECODE_BYTE();
            break;
        case 0x1E: 
            pylw("mvi e, ");
            DECODE_BYTE();
            break;
        case 0x26: 
            pylw("mvi h, ");
            DECODE_BYTE();
            break;
        case 0x2E: 
            pylw("mvi l, ");
            DECODE_BYTE();
            break;
        case 0x36: 
            pylw("mvi m, ");
            DECODE_BYTE();
            break;
        case 0x00: 
            pylw("nop");
            break;
        case 0xB7: 
            pylw("ora a");
            break;
        case 0xB0: 
            pylw("ora b");
            break;
        case 0xB1: 
            pylw("ora c");
            break;
        case 0xB2: 
            pylw("ora d");
            break;
        case 0xB3: 
            pylw("ora e");
            break;
        case 0xB4: 
            pylw("ora h");
            break;
        case 0xB5: 
            pylw("ora l");
            break;
        case 0xB6: 
            pylw("ora m");
            break;
        case 0xF6: 
            pylw("ori ");
            DECODE_BYTE();
            break;
        case 0xD3: 
            pylw("out ");
            DECODE_BYTE();
            break;
        case 0xE9: 
            pylw("pchl");
            break;
        case 0xC1: 
            pylw("pop b");
            break;
        case 0xD1: 
            pylw("pop d");
            break;
        case 0xE1: 
            pylw("pop h");
            break;
        case 0xF1: 
            pylw("pop psw");
            break;
        case 0xC5: 
            pylw("push b");
            break;
        case 0xD5: 
            pylw("push d");
            break;
        case 0xE5: 
            pylw("push h");
            break;
        case 0xF5: 
            pylw("push psw");
            break;
        case 0x17: 
            pylw("ral");
            break;
        case 0x1F: 
            pylw("rar");
            break;
        case 0xD8: 
            pylw("rc");
            break;
        case 0xC9: 
            pylw("ret");
            break;
        case 0x20: 
            pylw("rim");
            break;
        case 0x07: 
            pylw("rlc");
            break;
        case 0xF8: 
            pylw("rm");
            break;
        case 0xD0: 
            pylw("rnc");
            break;
        case 0xC0: 
            pylw("rnz");
            break;
        case 0xF0: 
            pylw("rp");
            break;
        case 0xE8: 
            pylw("rpe");
            break;
        case 0xE0: 
            pylw("rpo");
            break;
        case 0x0F: 
            pylw("rrc");
            break;
        case 0xC7: 
            pylw("rst 0");
            break;
        case 0xCF: 
            pylw("rst 1");
            break;
        case 0xD7: 
            pylw("rst 2");
            break;
        case 0xDF: 
            pylw("rst 3");
            break;
        case 0xE7: 
            pylw("rst 4");
            break;
        case 0xEF: 
            pylw("rst 5");
            break;
        case 0xF7: 
            pylw("rst 6");
            break;
        case 0xFF: 
            pylw("rst 7");
            break;
        case 0xC8: 
            pylw("rz");
            break;
        case 0x9F: 
            pylw("sbb a");
            break;
        case 0x98: 
            pylw("sbb b");
            break;
        case 0x99: 
            pylw("sbb c");
            break;
        case 0x9A: 
            pylw("sbb d");
            break;
        case 0x9B: 
            pylw("sbb e");
            break;
        case 0x9C: 
            pylw("sbb h");
            break;
        case 0x9D: 
            pylw("sbb l");
            break;
        case 0x9E: 
            pylw("sbb m");
            break;
        case 0xDE: 
            pylw("sbi ");
            DECODE_BYTE();
            break;
        case 0x22: 
            pylw("shld ");
            DECODE_WORD();
            break;
        case 0x30: 
            pylw("sim");
            break;
        case 0xF9: 
            pylw("sphl");
            break;
        case 0x32: 
            pylw("sta ");
            DECODE_WORD();
            break;
        case 0x02: 
            pylw("stax b");
            break;
        case 0x12: 
            pylw("stax d");
            break;
        case 0x37: 
            pylw("stc");
            break;
        case 0x97: 
            pylw("sub a");
            break;
        case 0x90: 
            pylw("sub b");
            break;
        case 0x91: 
            pylw("sub c");
            break;
        case 0x92: 
            pylw("sub d");
            break;
        case 0x93: 
            pylw("sub e");
            break;
        case 0x94: 
            pylw("sub h");
            break;
        case 0x95: 
            pylw("sub l");
            break;
        case 0x96: 
            pylw("sub m");
            break;
        case 0xD6: 
            pylw("sui ");
            DECODE_BYTE();
            break;
        case 0xEB: 
            pylw("xchg");
            break;
        case 0xAF: 
            pylw("xra a");
            break;
        case 0xA8: 
            pylw("xra b");
            break;
        case 0xA9: 
            pylw("xra c");
            break;
        case 0xAA: 
            pylw("xra d");
            break;
        case 0xAB: 
            pylw("xra e");
            break;
        case 0xAC: 
            pylw("xra h");
            break;
        case 0xAD: 
            pylw("xra l");
            break;
        case 0xAE: 
            pylw("xra m");
            break;
        case 0xEE: 
            pylw("xri ");
            DECODE_BYTE();
            break;
        case 0xE3: 
            pylw("xthl");
            break;
    }
}

#endif
