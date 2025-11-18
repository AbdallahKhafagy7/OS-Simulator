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
    int x = getClk();
    printf("Process with remaining time %d started at time %d\n", remainingtime, x);
    //raise(SIGSTOP);
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    while (remainingtime > 0)
    {
        // remainingtime = ??;
        remainingtime--;
        sleep(1);
    }
    x = getClk();
    printf("Process finished at time %d\n", x);
    exit(getpid());
    destroyClk(false);
    
    return 0;
}
