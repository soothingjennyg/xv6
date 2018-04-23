#ifdef CS333_P2
#include "types.h"
#include "user.h"
    int
main(int argc, char * argv[])
{
    int pid;
    int start;
    int finish;
    int total;
    int total_s;//total seconds
    int total_m;//total milliseconds
    //uptime is a system call

    //  struct rtcdate start;
    //  struct rtcdate finish;
    //  date(&start);
    if (argc == 1){
        printf(1, "Time ran in 0.00 seconds\n");
        exit();
        }    

    start = uptime();
    pid = fork();
    if (pid == 0){
        //exec(argv[1], &argv[2]);
        exec(argv[1], argv+1);
        printf(1, "The process did not run\n");
        exit();//only returns if fails
    }
    if (pid > 0){
        wait();
        finish = uptime();
        //date(&finish);
    }
    else {
        printf(1, "something bad happened in time function.\n");
        exit();
    }
    total = finish - start;
    total_s = total/1000;
    total_m = total%1000;
    if (total_m < 10){
    printf(1, "%s ran in %d.00%d seconds\n", argv[1],total_s, total_m);
}
    else if (total_m < 100){
    printf(1, "%s ran in %d.0%d seconds\n", argv[1], total_s, total_m);
}
    else{
    printf(1, "%s ran in %d.%d seconds\n", argv[1], total_s, total_m);
}
    exit();
}

#endif
/*
#ifdef CS333_P2
#include "types.h"
#include "user.h"
int
main(int argc, char * argv[])
{
uint pid;
int start;
int finish;
// uint total;
//uptime is a system call

//  struct rtcdate start;
//  struct rtcdate finish;
//  date(&start);
start = uptime();
pid = fork();
if (pid == 0){
exec(argv[1], &argv[2]);
}
if (pid > 0){
wait();
finish = uptime();
//date(&finish);
}
else {
//  printf(1, "something bad happened in time function.\n");
//  printf(1," %s", argv[0]);
//  printf(1, "Not imlpemented yet.\n");
}
//total = finish - start;
printf(1, "total time");
exit();
}

#endif
 */
