#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    remainingtime=atoi(argv[1]);
    int current_time = getClk();
    printf("Process with remaining time %d started at time %d\n", remainingtime, current_time);
    //raise(SIGSTOP);
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    int clock_timer=getClk();
    while (remainingtime > 0)
    {
        // remainingtime = ??;
        if(clock_timer!=getClk()){
            clock_timer=getClk();
            remainingtime--;
            printf("Process with remaining time %d at time %d\n", remainingtime,getClk());
        }
    }

    printf("Process finished at time %d\n", current_time);
    kill(getppid(),SIGUSR1);    
    exit(getpid());
    destroyClk(false);
    
    return 0;
}
