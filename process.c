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
    //raise(SIGSTOP);
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
  int clock_timer=getClk();
    printf("Process with remaining time %d started at time %d\n", remainingtime, clock_timer);
    
    while (remainingtime > 0)
    {
        int current_time = getClk();
        if(clock_timer != current_time){
            clock_timer = current_time;
            remainingtime--;
            printf("Process with remaining time %d at time %d\n", remainingtime, clock_timer);
        }
    }

    printf("Process finished at time %d\n", getClk());
    kill(getppid(), SIGUSR1);    
    destroyClk(false);
    return 0;
}
