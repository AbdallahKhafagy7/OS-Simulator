#include "headers.h"
#include <stdio.h>

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    remainingtime = atoi(argv[1]);
    printf("Process::%d\n",remainingtime);
    while (remainingtime > 0)
    {
        // remainingtime = ??;
        int t = getClk()+1;
        if (t==getClk())
        {
            remainingtime--;
            printf("Process: remaining time %d \n",remainingtime);
        }
    }
    
    destroyClk(false);
    
    return 0;
}
