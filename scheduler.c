#include "headers.h"
#include <stdio.h>

int MESSAGE_ID;

int main(int argc, char * argv[])
{
    initClk();
    
    //TODO implement the scheduler :)
    /*---------------------------Omar Syed------------------------------------*/
    int selected_Algorithm_NUM=atoi(argv[1]);
    int TIME_QUANTUM=atoi(argv[2]);
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    message_buf PROCESS_MESSAGE;
while(1){
if(msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf)-sizeof(long),0,0)==-1){
    printf("Error in receiving message!\n");
}
printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
    ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
}
/*---------------------------Omar Syed------------------------------------*/
    //upon termination release the clock resources.
    
    destroyClk(true);
}
