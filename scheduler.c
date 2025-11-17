#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_priority_queue.h"
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
PCB PCB_ARRAY[max]; 
process_queue WAITING_QUEUE;
process_queue READY_QUEUE;
process_priority_queue READY_PRIORITY_QUEUE;

/*---------------------------------Omar Syed------------------------------------*/
int main(int argc, char * argv[])
{
    initClk();
    
    //TODO implement the scheduler :)
    /*---------------------------Omar Syed------------------------------------*/
    initialize_queue(&WAITING_QUEUE);
    initialize_queue(&READY_QUEUE);
    initialize_priority_queue(&READY_PRIORITY_QUEUE);
    int selected_Algorithm_NUM=atoi(argv[1]);
    int TIME_QUANTUM=atoi(argv[2]);// if RR 3
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
    if( selected_Algorithm_NUM==3)//RR
    {
            
            printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
                ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
                PCB_ARRAY[process_count].p=PROCESS_MESSAGE.p;
                PCB_ARRAY[process_count].REMAINING_TIME=PROCESS_MESSAGE.p.RUNNING_TIME;
                PCB_ARRAY[process_count].process_state="Ready";
                PCB_ARRAY[process_count].RUNNING_TIME=0;
                PCB_ARRAY[process_count].START_TIME=-1;
                PCB_ARRAY[process_count].LAST_EXECUTED_TIME=-1;
                PCB_ARRAY[process_count].FINISH_TIME=-1;
                PCB_ARRAY[process_count].is_completed=false;
        enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
        process_count++;
        }
        else if(selected_Algorithm_NUM==1 ||selected_Algorithm_NUM==2)//HPF or SRTN
        {
            
            printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
                ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
                PCB_ARRAY[process_count].p=PROCESS_MESSAGE.p;
                PCB_ARRAY[process_count].REMAINING_TIME=PROCESS_MESSAGE.p.RUNNING_TIME;
                PCB_ARRAY[process_count].RUNNING_TIME=0;
                PCB_ARRAY[process_count].START_TIME=-1;
                PCB_ARRAY[process_count].LAST_EXECUTED_TIME=-1;
                PCB_ARRAY[process_count].FINISH_TIME=-1;
                PCB_ARRAY[process_count].process_state="Ready";
                PCB_ARRAY[process_count].is_completed=false;
            enqueue_priority(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);
            process_count++;
        }
        int RUNNING_PROCESS_PID = fork();
        if (RUNNING_PROCESS_PID == 0)
        {
            char PROCESS_REMAINGING_TIME_str[100];
            sprintf(PROCESS_REMAINGING_TIME_str, "%d", PROCESS_MESSAGE.p.ID);//i can send it by msg also
            execl("./process.out", "process.out",PROCESS_REMAINGING_TIME_str, NULL);
            perror("Error in executing process");
            exit(1);    
        }
        else
        {
            if( selected_Algorithm_NUM==3)//RR
            {   
            int t = getClk();
            PCB_ARRAY[process_count-1].START_TIME=t;
            PCB_ARRAY[process_count-1].LAST_EXECUTED_TIME=t;
            PCB_ARRAY[process_count-1].process_state="Running";
            if(PCB_ARRAY[process_count-1].START_TIME+t==TIME_QUANTUM){
                kill(RUNNING_PROCESS_PID,SIGSTOP);
                dequeue(&READY_QUEUE);
                PCB_ARRAY[process_count-1].REMAINING_TIME-=PCB_ARRAY[process_count-1].RUNNING_TIME-TIME_QUANTUM;
                PCB_ARRAY[process_count-1].process_state="Ready";
                PCB_ARRAY[process_count-1].LAST_EXECUTED_TIME=getClk();
                enqueue(&READY_QUEUE,PCB_ARRAY[process_count-1].p);
                if (PCB_ARRAY[process_count-1].REMAINING_TIME<=0){
                    PCB_ARRAY[process_count-1].is_completed=true;
                    PCB_ARRAY[process_count-1].FINISH_TIME=getClk();
                    PCB_ARRAY[process_count-1].process_state="Terminated";
                    kill(RUNNING_PROCESS_PID,SIGCONT);
                    waitpid(RUNNING_PROCESS_PID,NULL,0);
                }
            }
            else if(selected_Algorithm_NUM==1 ||selected_Algorithm_NUM==2)//HPF or SRTN
            {   
            int t = getClk();
            //TO be Implemented
            }
        }
        }
}
}
/*---------------------------Omar Syed------------------------------------*/
    //upon termination release the clock resources.
    
    destroyClk(true);
}
