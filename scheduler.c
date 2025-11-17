#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_queue.h"
#include "headers.h"
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
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
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    message_buf PROCESS_MESSAGE;
    int process_count=0;
    /*---------------------------Omar Syed------------------------------------*/
while(1){
if(msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT)!=-1){
    
    printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
        ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
        PCB_ARRAY[process_count].p=PROCESS_MESSAGE.p;
        PCB_ARRAY[process_count].REMAINING_TIME=PROCESS_MESSAGE.p.RUNNING_TIME;
        PCB_ARRAY[process_count].process_state="Ready";
        PCB_ARRAY[process_count].is_completed=false;
        enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
        process_count++;
        int RUNNING_PROCESS_PID = fork();
        if (RUNNING_PROCESS_PID == 0)
        {
            char PROCESS_REMAINGING_TIME_str[100];
            sprintf(PROCESS_REMAINGING_TIME_str, "%d", PROCESS_MESSAGE.p.ID);
            execl("./process.out", "process.out",PROCESS_REMAINGING_TIME_str, NULL);
            perror("Error in executing process");
            exit(1);    
        }
        else
        {
            key_t MESSAGE_PROCESS_KEY = ftok("keyfile", 'B');
            //for communication wth process
            int PROCESS_MESSAGE_ID = msgget(MESSAGE_PROCESS_KEY, 0666 | IPC_CREAT);
            if (PROCESS_MESSAGE_ID == -1)
            {
                printf("Error In Creating Message Queue For Process!\n");   
            }

        
    }
}
}
/*---------------------------Omar Syed------------------------------------*/
    //upon termination release the clock resources.
    
    destroyClk(true);
}
