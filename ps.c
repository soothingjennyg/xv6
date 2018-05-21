#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "uproc.h"

#define MAX_PROCS 32
    int
main(void)
{
    //  printf(1, "Not imlpemented yet.\n");
    struct uproc r[MAX_PROCS];
    int total;
    int elapsed_ticks_secs;
    int elapsed_ticks_ms;
    int total_ticks_secs;
    int total_ticks_ms;

    total = getprocs(MAX_PROCS,r);

    printf(1, "PID\tPUID\tPGID\tPPID\tPrio\telapsed time\ttotal CPU time\tstate\tsize\tname\n");
    for (int i = 0; i < total; ++i){
        elapsed_ticks_secs = r[i].elapsed_ticks/1000;
        elapsed_ticks_ms = r[i].elapsed_ticks%1000;
        total_ticks_secs = r[i].CPU_total_ticks/1000;
        total_ticks_ms = r[i].CPU_total_ticks%1000;
      //printf(1, "total ticks %d   CPU  %d\n", total_ticks_secs, r[i].CPU_total_ticks);  
       if (elapsed_ticks_ms < 10){
            printf(1, "%d\t %d\t %d\t %d\t %d\t %d.00%d\t\t",r[i].pid, r[i].uid, r[i].gid, r[i].ppid, r[i].prio, elapsed_ticks_secs, elapsed_ticks_ms);
        }
        else if (elapsed_ticks_ms < 100){
            printf(1, "%d\t %d\t %d\t %d\t %d\t %d.0%d\t\t",r[i].pid, r[i].uid, r[i].gid, r[i].ppid, r[i].prio, elapsed_ticks_secs, elapsed_ticks_ms);
        }
        else {
            printf(1, "%d\t %d\t %d\t %d\t %d\t %d.%d\t\t",r[i].pid, r[i].uid, r[i].gid, r[i].ppid, r[i].prio, elapsed_ticks_secs, elapsed_ticks_ms);
        }
        if (total_ticks_ms < 10){
            printf(1, "%d.00%d\t\t%s\t %d\t %s\n",total_ticks_secs, total_ticks_ms, r[i].state, r[i].size, r[i].name);
        }
        else if (total_ticks_ms < 100){
            printf(1, "%d.0%d\t\t%s\t %d\t %s\n",total_ticks_secs, total_ticks_ms, r[i].state, r[i].size, r[i].name);
        }
        else {
            printf(1, "%d.%d\t\t%s\t %d\t %s\n",total_ticks_secs, total_ticks_ms, r[i].state, r[i].size, r[i].name);
        }
    } 
    //call from sysproc.c
    exit();
}
#endif
