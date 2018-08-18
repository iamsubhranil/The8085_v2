#include <memory.h>
#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "display.h"
#include "test.h"
#include "util.h"
#include "vm.h"

static void reset_machine(Machine *m, u8 *memory, u16 size){
    memset(memory, 0, size);
    m->pc = 0;
    memset(m->registers, 0, 8);
    m->pc = 0;
    m->sp = 0xffff - 1;
}

static bool run_source(const char *source, Machine *m, u8 *memory, u16 size, u16 pointer){
    CompilationStatus status;
    compiler_reset();
    if((status = compile(source, &memory[0], size, &pointer)) != COMPILE_OK){
        pred("\n[compilation aborted with code %d]", status);
        return false;
    }
    run(m, &memory[0], 0);
    return true;
}

#define TEST(name)                                      \
    total_count++;                                      \
    testname = strdup(#name);                           \
    printf("\r%*.s", 50, " ");                          \
    phylw("\r[Test] ", "%-4s", #name);                  \
    failed = false;                                     \
    reset_machine(&m, &memory[0], size);                \
    source = readFile("test/" #name ".8085");           \
    if(!run_source(source, &m, &memory[0], size, 0)){   \
        pred(" [failed]");                              \
        failed = true;                                  \
    }

#define EXPECT(target, value)                                                   \
    if(target != value){                                                        \
        pred("\n[Test:%s] ", testname);                                         \
        printf(#target " -> expected : 0x%x, received : 0x%x\n", value, target);\
        failed = true;                                                          \
    }

#define DECIDE()            \
    free(source);           \
    free(testname);         \
    if(failed){             \
        pred(" [failed]");  \
        fail_count++;       \
    }                       \
    else{                   \
        pgrn(" [passed]");  \
        pass_count++;       \
    }

#define SHOW_STAT()                                         \
    pylw("\r[Test] ");                                      \
    printf("Total %" PRIu8 " tests done", total_count);     \
    printf(" [Passed : ");                                  \
    pgrn("%" PRIu8, pass_count);                            \
    printf(", Failed : ");                                  \
    pred("%" PRIu8, fail_count);                            \
    printf("]");

#define reg(r)  m.registers[r]
#define ra      reg(REG_A)
#define rb      reg(REG_B)
#define rc      reg(REG_C)
#define rd      reg(REG_D)
#define re      reg(REG_E)
#define rh      reg(REG_H)
#define rl      reg(REG_L)

#define pc      m.pc
#define sp      m.sp

#define flag(f) ((m.registers[REG_FL] >> f) & 1)
#define fls     flag(FLG_S)
#define flz     flag(FLG_Z)
#define fla     flag(FLG_A)
#define flp     flag(FLG_P)
#define flc     flag(FLG_C)

void test_all(){
    Machine m;
    u8 memory[0xffff];
    u16 size = 0xffff;
    char *source  = NULL;
    bool failed = false;
    char *testname = NULL;
    u8 total_count = 0, pass_count = 0, fail_count = 0;

    // All tests are sorted in the order of dependency

    TEST(lxi);
    EXPECT(rb, 0x45);
    EXPECT(rc, 0x67);
    EXPECT(rd, 0x89);
    EXPECT(re, 0xab);
    EXPECT(rh, 0xcd);
    EXPECT(rl, 0xef);
    DECIDE();

    TEST(mvi);
    EXPECT(ra, 0x05);
    EXPECT(rb, 0x02);
    EXPECT(rc, 0x06);
    EXPECT(rd, 0x09);
    EXPECT(re, 0x00);
    EXPECT(rh, 0x10);
    EXPECT(rl, 0x10);
    EXPECT(memory[0x1010], 0xba);
    DECIDE();

    TEST(mov);
    EXPECT(ra, 0x87);
    EXPECT(rb, 0x06);
    EXPECT(memory[0x0600], 0xab);
    EXPECT(rl, 0xab);
    DECIDE();

    TEST(cmc);
    EXPECT(flc, 0x01);
    DECIDE();

    TEST(stc);
    EXPECT(flc, 0x01);
    DECIDE();

    TEST(ani);
    EXPECT(ra, 0x00);
    DECIDE();

    TEST(ana);
    EXPECT(rb, 0x89);
    EXPECT(ra, 0x09);
    DECIDE();

    TEST(ori);
    EXPECT(ra, 0x39);
    DECIDE();

    TEST(ora);
    EXPECT(rb, 0x5a);
    EXPECT(ra, 0x5b);
    DECIDE();

    TEST(xri);
    EXPECT(ra, 0xb6);
    DECIDE();

    TEST(xra);
    EXPECT(rb, 0x05);
    EXPECT(ra, 0x35);
    DECIDE();

    TEST(adi);
    EXPECT(ra, 0x13);
    EXPECT(flc, 0x01);
    DECIDE();

    TEST(aci);
    EXPECT(ra, 0x02);
    EXPECT(flc, 0x00);
    DECIDE();

    TEST(add);
    EXPECT(rb, 0x41);
    EXPECT(ra, 0xa1);
    EXPECT(flc, 0x00);
    DECIDE();

    TEST(adc);
    EXPECT(rb, 0x53);
    EXPECT(ra, 0xbb);
    EXPECT(flc, 0x00);
    DECIDE();

    TEST(sui);
    EXPECT(ra, 0x2f);
    EXPECT(flc, 0x00);
    DECIDE();

    TEST(sbi);
    EXPECT(ra, 0x11);
    EXPECT(flc, 0x00);
    DECIDE();

    TEST(sub);
    EXPECT(rb, 0xdc);
    EXPECT(ra, 0x49);
    EXPECT(flc, 0x00);
    DECIDE();

    TEST(sbb);
    EXPECT(rb, 0xf7);
    EXPECT(ra, 0xf7);
    EXPECT(flp, 0x00);
    EXPECT(flc, 0x01);
    DECIDE();

    TEST(inr);
    EXPECT(ra, 0x44);
    EXPECT(rb, 0x2a);
    EXPECT(rc, 0x33);
    EXPECT(rd, 0x76);
    EXPECT(re, 0x49);
    EXPECT(rh, 0x39);
    EXPECT(rl, 0x1a);
    EXPECT(memory[0x391a], 0x48);
    EXPECT(flp, 0x01);
    DECIDE();

    TEST(inx);
    EXPECT(rb, 0x29);
    EXPECT(rc, 0x44);
    EXPECT(rh, 0x18);
    EXPECT(rl, 0x1a);
    EXPECT(rd, 0x14);
    EXPECT(re, 0x00);
    DECIDE();

    TEST(dcr);
    EXPECT(ra, 0x42);
    EXPECT(rb, 0x27);
    EXPECT(rc, 0x82);
    EXPECT(rd, 0x83);
    EXPECT(re, 0x22);
    EXPECT(rh, 0x93);
    EXPECT(rl, 0xff);
    EXPECT(memory[0x93ff], 0x82);
    EXPECT(flp, 0x01);
    DECIDE();

    TEST(dcx);
    EXPECT(rb, 0x73);
    EXPECT(rc, 0x91);
    EXPECT(rh, 0x48);
    EXPECT(rl, 0x17);
    EXPECT(rd, 0x39);
    EXPECT(re, 0x2f);
    DECIDE();

    TEST(call);
    EXPECT(ra, 0x36);
    EXPECT(sp, 0xffff - 3);
    DECIDE();

    TEST(cc);
    EXPECT(ra, 0xab);
    EXPECT(flc, 0x01);
    EXPECT(sp, 0xffff - 3);
    DECIDE();

    TEST(cnc);
    EXPECT(ra, 0xcd);
    EXPECT(flc, 0x00);
    EXPECT(sp, 0xffff - 3);
    DECIDE();

    TEST(cp);
    EXPECT(ra, 0x74);
    EXPECT(sp, 0xffff - 3);
    DECIDE();

    TEST(cm);
    EXPECT(ra, 0x28);
    EXPECT(sp, 0xffff - 3);
    DECIDE();

    TEST(cz);
    EXPECT(ra, 0x00);
    EXPECT(flz, 0x01);
    EXPECT(sp, 0xffff - 3);
    DECIDE();

    TEST(cnz);
    EXPECT(ra, 0xab);
    EXPECT(flz, 0x00);
    EXPECT(sp, 0xffff - 3);
    DECIDE();

    TEST(cpe);
    EXPECT(ra, 0x37);
    EXPECT(flp, 0x01);
    EXPECT(sp, 0xffff - 3);
    DECIDE();

    TEST(cpo);
    EXPECT(ra, 0x43);
    EXPECT(flp, 0x00);
    EXPECT(sp, 0xffff - 3);
    DECIDE();

    TEST(ret);
    EXPECT(ra, 0x82);
    EXPECT(sp, 0xfffe);
    DECIDE();

    TEST(rc);
    EXPECT(ra, 0x87);
    EXPECT(flc, 0x01);
    EXPECT(sp, 0xfffe);
    DECIDE();

    TEST(rnc);
    EXPECT(ra, 0x37);
    EXPECT(flc, 0x00);
    EXPECT(sp, 0xfffe);
    DECIDE();

    TEST(rpe);
    EXPECT(ra, 0x05);
    EXPECT(flp, 0x01);
    EXPECT(sp, 0xfffe);
    DECIDE();

    TEST(rpo);
    EXPECT(ra, 0x02);
    EXPECT(flp, 0x00);
    EXPECT(sp, 0xfffe);
    DECIDE();

    TEST(rz);
    EXPECT(ra, 0x00);
    EXPECT(flz, 0x01);
    EXPECT(sp, 0xfffe);
    DECIDE();

    TEST(rnz);
    EXPECT(ra, 0xbf);
    EXPECT(flz, 0x00);
    EXPECT(sp, 0xfffe);
    DECIDE();

    TEST(rp);
    EXPECT(ra, 0x7f);
    EXPECT(fls, 0x00);
    EXPECT(sp, 0xfffe);
    DECIDE();

    TEST(rm);
    EXPECT(ra, 0x80);
    EXPECT(fls, 0x01);
    EXPECT(sp, 0xfffe);
    DECIDE();
    
    SHOW_STAT();
}
