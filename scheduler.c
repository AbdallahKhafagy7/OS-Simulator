#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "headers.h"
//#include <cstddef>
#include <signal.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>

/*---------------------------------Omar Syed------------------------------------*/

int MESSAGE_ID;

/*---------------------------------QUEUES&PCB------------------------------------*/

//cpu bound --> no blocking queue
PCB PCB_ENTRY;
process_queue READY_QUEUE;
process_priority_queue READY_PRIORITY_QUEUE;


void UPDATE_READY_QUEUE(){
 if(!is_queue_empty(&READY_QUEUE)){
    process* temp=dequeue(&READY_QUEUE);
    enqueue(&READY_QUEUE,*temp);
 }
}

void UPDATE_READY_PRIORITY_QUEUE(){
 if(!is_priority_queue_empty(&READY_PRIORITY_QUEUE)){
    process* temp=dequeue_priority(&READY_PRIORITY_QUEUE);
    enqueue_priority(&READY_PRIORITY_QUEUE,*temp);
 }
}
void PRINT_READY_QUEUE(){
    process_Node* temp=READY_QUEUE.front;
    printf("Ready Queue: ");
    while(temp!=NULL){
        printf("P%d ",temp->Process.ID);
        temp=temp->next;
        if(temp==READY_QUEUE.rear->next){
            break;
        }
    }
    printf("\n");
}

void PRINT_READY_PRIORITY_QUEUE(){
    process_PNode* temp=READY_PRIORITY_QUEUE.front;
    printf("Ready Priority Queue: ");
    while(temp!=NULL){
        printf("P%d ",temp->Process.ID);
        temp=temp->next;
    }
    printf("\n");
}

process GET_PROCESS_READY_QUEUE(int index){
    if(!is_queue_empty(&READY_QUEUE)){
    process_Node* temp=READY_QUEUE.front;
    int count=0;
    while(temp!=NULL){
        if(count==index){
            return temp->Process;
        } 
        count++;
        temp=temp->next;
    }
}
}

process GET_PROCESS_READY_PRIORITY_QUEUE(int index){
    if(!is_priority_queue_empty(&READY_PRIORITY_QUEUE)){
    process_PNode* temp=READY_PRIORITY_QUEUE.front;
    int count=0;
    while(temp!=NULL){
        if(count==index){
            return temp->Process;
        } 
        count++;
        temp=temp->next;
    }
}
}

void CHECK_DEPENDECY_RR(process_queue* queue){
    
}

void CHECK_DEPENDENCY(process_priority_queue* queue){
    
}

void SWITCH_PROCESS_RR(process_queue* queue){
    
}

void SWITCH_PROCESS_SRTN(process_priority_queue* queue){
    
}

void SWITCH_PROCESS_HPF(process_priority_queue* queue){
    
}

int PID[max];
/*---------------------------------Omar Syed------------------------------------*/
int main(int argc, char * argv[])
{
    initClk();
    
    //TODO implement the scheduler :)
    /*---------------------------Omar Syed------------------------------------*/
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
    switch (selected_Algorithm_NUM)
    {
        case 1:
        {
            printf("Scheduling Algorithm is Highest Priority first\n");
            while(1){
                if(msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT)!=-1){
                    printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
                    ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
                    PCB_ENTRY.p=PROCESS_MESSAGE.p;
                    PCB_ENTRY.REMAINING_TIME=PROCESS_MESSAGE.p.RUNNING_TIME;
                    PCB_ENTRY.RUNNING_TIME=0;
                    PCB_ENTRY.START_TIME=-1;
                    PCB_ENTRY.LAST_EXECUTED_TIME=-1;
                    PCB_ENTRY.FINISH_TIME=-1;
                    PCB_ENTRY.process_state=Ready;
                    PCB_ENTRY.is_completed=false;
                    enqueue_priority(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);
                    PID[process_count] = fork();
                    if(PID[process_count] == 0){
                        char runningtime_str[max];
                        sprintf(runningtime_str, "%d", PROCESS_MESSAGE.p.RUNNING_TIME);
                        execl("./process.out", "process.out", runningtime_str, NULL);
                        perror("Failed to run process\n");
                        exit(1);
                    }
                    kill(PID[process_count], SIGSTOP);
                    process_count++;
                    
                    
                    //RINT_READY_PRIORITY_QUEUE();
                }
        }
        break;
        }
        case 2:
        {
            printf("Scheduling Algorithm is Shortest Remaining Time Next\n");
            while(1){
                if(msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT)!=-1){
                    printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
                    ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
                    PCB_ENTRY.p=PROCESS_MESSAGE.p;
                    PCB_ENTRY.REMAINING_TIME=PROCESS_MESSAGE.p.RUNNING_TIME;
                    PCB_ENTRY.RUNNING_TIME=0;
                    PCB_ENTRY.START_TIME=-1;
                    PCB_ENTRY.LAST_EXECUTED_TIME=-1;
                    PCB_ENTRY.FINISH_TIME=-1;
                    PCB_ENTRY.process_state=Ready;
                    PCB_ENTRY.is_completed=false;
                    enqueue_priority_SRTN(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);
                                        PID[process_count] = fork();
                    if(PID[process_count] == 0){
                        char runningtime_str[max];
                        sprintf(runningtime_str, "%d", PROCESS_MESSAGE.p.RUNNING_TIME);
                        execl("./process.out", "process.out", runningtime_str, NULL);
                        perror("Failed to run process\n");
                        exit(1);
                    }
                    kill(PID[process_count], SIGSTOP);
                    process_count++;
                    
                    //PRINT_READY_PRIORITY_QUEUE();
                }
        }
        break;
        }
        case 3:
        {
            printf("Scheduling Algorithm is Round Robin with Time Quantum = %d\n",TIME_QUANTUM);
            while(1){
                if(msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT)!=-1){
                    printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
                    ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
                    PCB_ENTRY.p=PROCESS_MESSAGE.p;
                    PCB_ENTRY.REMAINING_TIME=PROCESS_MESSAGE.p.RUNNING_TIME;
                    PCB_ENTRY.RUNNING_TIME=0;
                    PCB_ENTRY.START_TIME=-1;
                    PCB_ENTRY.LAST_EXECUTED_TIME=-1;
                    PCB_ENTRY.FINISH_TIME=-1;
                    PCB_ENTRY.process_state=Ready;
                    PCB_ENTRY.is_completed=false;
                    enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
                                        PID[process_count] = fork();
                    if(PID[process_count] == 0){
                        char runningtime_str[max];
                        sprintf(runningtime_str, "%d", PROCESS_MESSAGE.p.RUNNING_TIME);
                        execl("./process.out", "process.out", runningtime_str, NULL);
                        perror("Failed to run process\n");
                        exit(1);
                    }
                    kill(PID[process_count], SIGSTOP);
                    process_count++;
                    //PRINT_READY_QUEUE();
                }
        }
        break;
    }
    default:
    break;
}
/*---------------------------Omar Syed------------------------------------*/
/*
while(1){
if(msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT)!=-1){
    if( selected_Algorithm_NUM==3)//RR
    {
            
            printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
                ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
                PCB_ENTRY.p=PROCESS_MESSAGE.p;
                PCB_ENTRY.REMAINING_TIME=PROCESS_MESSAGE.p.RUNNING_TIME;
                PCB_ENTRY.process_state=Ready;
                PCB_ENTRY.RUNNING_TIME=0;
                PCB_ENTRY.START_TIME=-1;
                PCB_ENTRY.LAST_EXECUTED_TIME=-1;
                PCB_ENTRY.FINISH_TIME=-1;
                PCB_ENTRY.is_completed=false;
        enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
        process_count++;
        PRINT_READY_QUEUE();
    }
    if(selected_Algorithm_NUM ==1)//HPF 
    {
        
        printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
            ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
            PCB_ENTRY.p=PROCESS_MESSAGE.p;
            PCB_ENTRY.REMAINING_TIME=PROCESS_MESSAGE.p.RUNNING_TIME;
            PCB_ENTRY.RUNNING_TIME=0;
            PCB_ENTRY.START_TIME=-1;
            PCB_ENTRY.LAST_EXECUTED_TIME=-1;
            PCB_ENTRY.FINISH_TIME=-1;
            PCB_ENTRY.process_state=Ready;
            PCB_ENTRY.is_completed=false;
            enqueue_priority(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);
            process_count++;
            PRINT_READY_PRIORITY_QUEUE();
        }
    if(selected_Algorithm_NUM ==2)//SRTN
    {
        printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
            ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
            PCB_ENTRY.p=PROCESS_MESSAGE.p;
            PCB_ENTRY.REMAINING_TIME=PROCESS_MESSAGE.p.RUNNING_TIME;
            PCB_ENTRY.RUNNING_TIME=0;
            PCB_ENTRY.START_TIME=-1;
            PCB_ENTRY.LAST_EXECUTED_TIME=-1;
            PCB_ENTRY.FINISH_TIME=-1;
            PCB_ENTRY.process_state=Ready;
            PCB_ENTRY.is_completed=false;
            enqueue_priority_SRTN(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);
            process_count++;
            PRINT_READY_PRIORITY_QUEUE();
        }
}
*/
//upon termination release the clock resources.

destroyClk(true);

}
