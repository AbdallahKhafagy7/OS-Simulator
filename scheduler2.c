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
/*
1-Receive processes
2-Initialize Queues & PCB
3-Initialize Algorithms
Done by Omar Syed
*/
/*---------------------------------QUEUES&PCB------------------------------------*/

//cpu bound --> no blocking queue
PCB PCB_ENTRY;
process_queue READY_QUEUE;
process_priority_queue READY_PRIORITY_QUEUE;


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

int PID[max];
/*---------------------------------Omar Syed------------------------------------*/
int main(int argc, char * argv[])
{
    int clock_timer= 0;
    initClk();
    
    //TODO implement the scheduler :)
    /*---------------------------Omar Syed------------------------------------*/
    initialize_queue(&READY_QUEUE);
    initialize_priority_queue(&READY_PRIORITY_QUEUE);
    int selected_Algorithm_NUM=atoi(argv[1]);
    int TIME_QUANTUM=atoi(argv[2]);// if RR 3
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    printf("queue id  is: %d\n",MESSAGE_ID);
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
            int rec_status = msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT);
            printf("Scheduling Algorithm is Highest Priority first\n");
            while(1){
                if(clock_timer<getClk()){
                    clock_timer=getClk();
                    if(rec_status!=-1){ // when getting a new process
                        printf("clock now at rec is: %d\n",clock_timer);
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
                        else
                        {
                            printf("kjdasd");
                        }

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
                    PRINT_READY_PRIORITY_QUEUE();
                }
        }
        break;
        }
        case 3:
        {
            printf("Scheduling Algorithm is Round Robin with Time Quantum = %d\n",TIME_QUANTUM);
            int firsttime =true; // TODO::fix this to be when queue is empty
            process_Node* current_process;
            while(1){
                int rec_status = msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT);
                if(rec_status!=-1)
                {
                        printf("clock now is: %d\n",clock_timer);
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
                    process_count++;
                    PRINT_READY_QUEUE();
                }
                if(clock_timer!=getClk())
                {
                    clock_timer=getClk();
                
                    printf("Clock Timer: %d\n",clock_timer);
                    if (firsttime)
                    {
                                printf("started");
                                current_process=peek_front(&READY_QUEUE);
                                if(current_process!=NULL){
                                    printf("At time %d, Process P%d is running\n",clock_timer,current_process->Process.ID);
                                    firsttime=false;
                                    }
                    }
                    if (clock_timer%TIME_QUANTUM==0)
                    {  // a time quantum has passed
                                kill(current_process->Process.ID,SIGSTOP); //TODO:: FORK THE PROCCES THEN KILL IT
                                current_process=current_process->next;
                                if(current_process==NULL)
                                {
                                    current_process=READY_QUEUE.front;
                                }
                                printf("At time %d, Process P%d is running\n",clock_timer,current_process->Process.ID);
                    }
                    firsttime=is_queue_empty(&READY_QUEUE);
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

 enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
                            PRINT_READY_QUEUE();
                            if (get_count(&READY_QUEUE)==1 && 
                            peek_front(&READY_QUEUE)->Process.first_time==true )
                            {
                                char remaining_time_string[20] ;
                                sprintf(remaining_time_string,"%d",peek_front(&READY_QUEUE)->Process.RUNNING_TIME );
                                PCB pcb_obj ;
                                pcb_obj.arrival_time=peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                                pcb_obj.LAST_EXECUTED_TIME=getClk();
                                pcb_obj.REMAINING_TIME=peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                                pcb_obj.process_state=Running;
                                pcb_obj.START_TIME=getClk();
                                pcb_obj.process_id=peek_front(&READY_QUEUE)->Process.ID;
                                pcb_obj.is_completed=false;
                                int pid =fork();
                                if(pid==0){
                                    execl("./process.out","./process.out",remaining_time_string, NULL);
                                    perror("Error In forking");
                                }
                                else{
                                    pcb_obj.process_pid=pid;
                                    ADD_PCB(pcb, pcb_obj);
                                }
                            }
                                

                            void handler(int signum){

    PCB* to_be_removed_pcb =  get_PCB_entry(pcb,peek_front(&READY_QUEUE)->Process.ID );
    to_be_removed_pcb->process_state=Finished;
    to_be_removed_pcb->FINISH_TIME=getClk();
    to_be_removed_pcb->is_completed=true;
     pFile = fopen("processes.txt", "w");
    if (!pFile) {
        printf("Error opening file.\n");
    }
     fprintf(pFile, "%-5s %-10s %-10s %-10s %-20s %-5s %-10s %-10s %-10s %-20s %-10s %-20s\n",
        "#At", "time", "x", "process", "y","state","arr","w","total","z","remain","wait");
        fprintf(pFile, "%-5s %-10s %-10d %-10s %-20d %-5s %-10s %-10d %-10s %-20d %-10s %-20d %-20s %-10d\n",
        "#At", "time", to_be_removed_pcb->START_TIME, "process", to_be_removed_pcb->process_id,"Finished","arr",to_be_removed_pcb->arrival_time,"total",to_be_removed_pcb->RUNNING_TIME,"remain",to_be_removed_pcb->REMAINING_TIME,"wait",to_be_removed_pcb->LAST_EXECUTED_TIME-to_be_removed_pcb->arrival_time);
    fclose(pFile);
    Remove_PCB(pcb, peek_front(&READY_QUEUE)->Process.ID);
    printf("\nProcess with id %d has been dequeued\n",peek_front(&READY_QUEUE)->Process.ID);
    dequeue(&READY_QUEUE);
}