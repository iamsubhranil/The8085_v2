#include <time.h>

#include "compiler.h"
#include "display.h"
#include "vm.h"

// On a n Hz machine, this delay should be equal to about
// (35/n) + (7/n) + (14/n)*(<content of register c>) s
static const char* sub_delay = 
"       mvi b, 00h\n"
"next:  dcr b\n"
"       mvi c, 0ffh\n"
"delay: dcr c\n"
"       jnz delay\n"
"       mov a, b\n"
"       out 1h\n"
"       hlt\n";

// The number of times the test should be performed
// before settling on an average
#define RUNCOUNT 100
// The frequency of the machine in Hz
#define MACHINE_FREQ 3000000.0

#define total_tstates (35.0 + 7.0 + (14.0 * 0xff))
static double required_time = (total_tstates/MACHINE_FREQ);

void calibrate(Machine *m){
    (void)m;
    Machine cm = {{0}, 0, 0xffff, {0}, 0, 0, 1, m->sleepfor};
    u8 memory[0xff];
    u16 pointer = 0;
    compiler_reset();
    compile(sub_delay, memory, 0xff, &pointer);
    double req_tm_per_tstate = required_time/total_tstates;
    pinfo("Estimated time : %lfs (%lfs/run) (%.10lfs/t-state) (%lf mHz)", 
            required_time * RUNCOUNT, required_time, req_tm_per_tstate, MACHINE_FREQ/1000000);
    double total = 0;
    int runcount = RUNCOUNT;
    while(runcount--){
        clock_t start = clock();
        // Run delay
        cm.pc = 0;
        run(&cm, memory, 0);
        clock_t end = clock();
        double sec = (double)(end - start) / CLOCKS_PER_SEC;
        //pinfo("Delay took %f seconds", sec);
        total += sec;
    }
    double avg_tm_per_tstate = total/(total_tstates*RUNCOUNT);
    double sleepfor = 0;

    pinfo("[Before] Total time : %lfs (%lfs/run) (%.10lfs/t-state) (%lf mHz)", 
            total, RUNCOUNT, total/RUNCOUNT, avg_tm_per_tstate, (total_tstates * RUNCOUNT)/(total * 1000000));

    if(avg_tm_per_tstate < req_tm_per_tstate){
        sleepfor = req_tm_per_tstate - avg_tm_per_tstate;

        //double calmhz = (total_tstates * RUNCOUNT) / total;
        //if(calmhz > MACHINE_FREQ)
        //    sleepfor = 1/(calmhz - MACHINE_FREQ);

        cm.sleepfor.tv_sec = (time_t)sleepfor;
        cm.sleepfor.tv_nsec = m->sleepfor.tv_nsec + (long)(sleepfor * 1000000000);

        pinfo("Adjusting sleep for %ldns(%.10lfs)..", cm.sleepfor.tv_nsec, sleepfor);
        m->sleepfor = cm.sleepfor;

        total = 0;
        runcount = RUNCOUNT;
        while(runcount--){
            clock_t start = clock();
            // Run delay
            cm.pc = 0;
            run(&cm, memory, 0);
            clock_t end = clock();
            double sec = (double)(end - start) / CLOCKS_PER_SEC;
            //pinfo("Delay took %f seconds", sec);
            total += sec;
        }
        avg_tm_per_tstate = total/(total_tstates*RUNCOUNT);

        pinfo("[After] Total time : %lfs (%lfs/run) (%.10lfs/t-state) (%lf mHz)", 
                total, RUNCOUNT, total/RUNCOUNT, avg_tm_per_tstate, (total_tstates * RUNCOUNT)/(total * 1000000));
    }
    else
        pinfo("Not enough frequency delta to calibrate!");
}
