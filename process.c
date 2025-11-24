#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    remainingtime = atoi(argv[1]);
    int start_time = getClk();
    int clock_timer=getClk();
    
   // printf("Process with remaining time %d started at time %d\n", remainingtime, start_time);
    
/*    while (getClk() - start_time < remainingtime)
    {
    }*/
    while (remainingtime > 0)
    {
        // remainingtime = ??;
        if(clock_timer!=getClk()){
            clock_timer=getClk();
            remainingtime--;
           // printf("Process with remaining time %d at time %d\n", remainingtime,getClk());
        }
    }
     //printf("Process finished at time %d\n", getClk());


   // printf("Process finished at time %d\n", getClk());
    kill(getppid(), SIGUSR1);    
    destroyClk(false);
    return 0;
}