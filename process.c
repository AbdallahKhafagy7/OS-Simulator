#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <sys/msg.h>
#include <unistd.h>

int remainingtime;
//int msg_ID;
int flag =false;
void handler(int signum){
flag = true;
}
int main(int agrc, char * argv[])
{
    signal(SIGUSR2,handler);
    initClk();
    remainingtime = atoi(argv[1]);
    int start_time = getClk();
    int clock_timer=getClk();
   // key_t key_sch_process = ftok("keyfile_sch", 'B');
   // msg_ID = msgget(key_sch_process, 0600|IPC_CREAT);
   // scheduler_process_message finish_msg;
   // printf("Process with remaining time %d started at time %d\n", remainingtime, start_time);
    
/*    while (getClk() - start_time < remainingtime)
    {
    }*/
    while (remainingtime > 0)
    {
        // remainingtime = ??;
        // if(clock_timer!=getClk()){
        //     clock_timer=getClk();
        //     remainingtime--;
        //     printf("At time %d process with remaining time %d\n", getClk(),remainingtime);
        // }
       // if(msgrcv(msg_ID, &finish_msg, sizeof(scheduler_process_message), 5, 0)!=-1){
       //     printf("Process Terminating \n");
       //     if(finish_msg.flag==true)
       //     break;
       // }
       if(flag==true)
       break;
    }
     //printf("Process finished at time %d\n", getClk());


   // printf("Process finished at time %d\n", getClk()); 
    kill(getppid(),SIGUSR1); //--> youssef cov
    destroyClk(false);
    return 0;
}