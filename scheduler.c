#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_queue.h"
#include "headers.h"
#include <stdio.h>
#include <sys/ipc.h>
#include <unistd.h>

/*---------------------------------Omar Syed------------------------------------*/

int MESSAGE_ID;

/*---------------------------------QUEUES&PCB------------------------------------*/

//cpu bound --> no blocking queue
process_queue WAITING_QUEUE;
process_queue READY_QUEUE;
PCB PCB_ARRAY[max]; 

/*---------------------------------Omar Syed------------------------------------*/
int main(int argc, char * argv[])
{
    initClk();
    
    //TODO implement the scheduler :)
    /*---------------------------Omar Syed------------------------------------*/
    initialize_queue(&WAITING_QUEUE);
    initialize_queue(&READY_QUEUE);
    int selected_Algorithm_NUM=atoi(argv[1]);
    int TIME_QUANTUM=atoi(argv[2]);
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    message_buf PROCESS_MESSAGE;
    int process_count=0;
    /*---------------------------Omar Syed------------------------------------*/
while(1){
if(msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf)-sizeof(long),0,0)==-1){
    printf("Error in receiving message!\n");
}
printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
    ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
    PCB_ARRAY[process_count].p=PROCESS_MESSAGE.p;
    PCB_ARRAY[process_count].REMAINING_TIME=PROCESS_MESSAGE.p.RETURN_CODE;
    PCB_ARRAY[process_count].process_state="Ready";
    PCB_ARRAY[process_count].is_completed=false;
    enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
    process_count++;
    usleep(100);
}
/*---------------------------Omar Syed------------------------------------*/
    //upon termination release the clock resources.
    
    destroyClk(true);
}
