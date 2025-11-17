#include "headers.h"
#include <stdio.h>

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    printf("Process %d started at time %d with remaining time %d\n",atoi(argv[1]),getClk(),atoi(argv[2]));
        // remainingtime = ??;
    
    destroyClk(false);
    
    return 0;
}
