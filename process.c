#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <sys/msg.h>
#include <unistd.h>

int remainingtime;
int flag = 0;

void handler(int signum){
    flag = 1;
}

int main(int argc, char * argv[])
{
    signal(SIGUSR2, handler);
    initClk();
    remainingtime = atoi(argv[1]);
    int start_time = getClk();
    
    while (remainingtime > 0 && flag == 0)
    {
        int current = getClk();
        if (current > start_time) {
            int elapsed = current - start_time;
            remainingtime = atoi(argv[1]) - elapsed;
        }
    }
    
    destroyClk(false);
    return 0;
}