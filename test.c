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
    if((status = compile(source, &memory[0], size, &pointer)) != COMPILE_OK){
        pred("[compilation aborted with code %d]", status);
        return false;
    }
    run(m, &memory[0]);
    return true;
}

#define TEST(name)                                      \
    testname = strdup(#name);                           \
    phylw("\n[Test] ", "%s", #name);                    \
    failed = false;                                     \
    reset_machine(&m, &memory[0], size);                \
    source = readFile("test/" #name ".8085");           \
    if(!run_source(source, &m, &memory[0], size, 0)){   \
        pred(" [failed]");                              \
        failed = true;                                  \
    }

#define EXPECT(target, value)                                                  \
    if(target != value){                                                        \
        pred("\n[Test:%s] ", testname);                                         \
        printf(#target " -> expected : " #value ", received : 0x%x", target);   \
        failed = true;                                                          \
    }

#define DECIDE()            \
    free(source);           \
    free(testname);         \
    if(failed)              \
        pred(" [failed]");  \
    else                    \
        pgrn(" [passed]");

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
}
