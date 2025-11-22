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
    while (remainingtime > 0)
    {
        // remainingtime = ??;
        if(getClk()-1==current_time){
            remainingtime--;
            printf("Process with remaining time %d at time %d\n", remainingtime,getClk());
            current_time = getClk();
        }
    }

    printf("Process finished at time %d\n", current_time);
    exit(getpid());
    destroyClk(false);
    
    return 0;
}
