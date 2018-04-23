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

    printf(1, "PID\tPUID\tPGID\tPPID\telapsed time\ttotal CPU time\t state\t size\tname\n");
    for (int i = 0; i < total; ++i){
        elapsed_ticks_secs = r[i].elapsed_ticks/1000;
        elapsed_ticks_ms = r[i].elapsed_ticks%1000;
        total_ticks_secs = r[i].CPU_total_ticks/1000;
        total_ticks_ms = r[i].CPU_total_ticks%1000;
      //printf(1, "total ticks %d   CPU  %d\n", total_ticks_secs, r[i].CPU_total_ticks);  
       if (elapsed_ticks_ms < 10){
            printf(1, "%d\t %d\t %d\t %d\t %d.00%d\t\t",r[i].pid, r[i].uid, r[i].gid, r[i].ppid, elapsed_ticks_secs, elapsed_ticks_ms);
        }
        else if (elapsed_ticks_ms < 100){
            printf(1, "%d\t %d\t %d\t %d\t %d.0%d\t\t",r[i].pid, r[i].uid, r[i].gid, r[i].ppid, elapsed_ticks_secs, elapsed_ticks_ms);
        }
        else {
            printf(1, "%d\t %d\t %d\t %d\t %d.%d\t\t",r[i].pid, r[i].uid, r[i].gid, r[i].ppid, elapsed_ticks_secs, elapsed_ticks_ms);
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
/*
// The code for dayofweek was obtained at:
// https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week

#ifdef CS333_P1
#include "types.h"
#include "user.h"
#include "date.h"

static char *months[] = {"NULL", "Jan", "Feb", "Mar", "Apr",
"May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char *days[] = {"Sun", "Mon", "Tue", "Wed",
"Thu", "Fri", "Sat"};

int
dayofweek(int y, int m, int d)
{
return (d+=m<3?y--:y-2,23*m/9+d+4+y/4-y/100+y/400)%7;
}

int
main(int argc, char *argv[])
{
int day;
struct rtcdate r;

if (date(&r)) {
printf(2,"Error: date call failed. %s at line %d\n",
__FILE__, __LINE__);
exit();
}

day = dayofweek(r.year, r.month, r.day);

printf(1, "%s %s %d", days[day], months[r.month], r.day);
printf(1, " ");
if (r.hour < 10) printf(1, "0");
printf(1, "%d:", r.hour);
if (r.minute < 10) printf(1, "0");
printf(1, "%d:", r.minute);
if (r.second < 10) printf(1, "0");
printf(1, "%d UTC %d\n", r.second, r.year);

exit();
}
#endif
 */
