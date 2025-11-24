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
    
    printf("Process with remaining time %d started at time %d\n", remainingtime, start_time);
    
    while (getClk() - start_time < remainingtime)
    {
    }

    printf("Process finished at time %d\n", getClk());
    kill(getppid(), SIGUSR1);    
    destroyClk(false);
    return 0;
}