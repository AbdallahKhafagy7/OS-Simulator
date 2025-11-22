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
int TIME_QUANTUM;
int pid[max];
int running_process_pid=-1;
int running_process_id=-1;
int next_process_pid=-1;
int next_process_id=-1;
int running_process_index=-1;
int process_count=0;
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


int get_count(process_queue* READY_QUEUE){
    process_Node* temp=READY_QUEUE->front;
    int count=0;
    while(temp!=NULL){
        count++;
        temp=temp->next;
        if(temp==READY_QUEUE->rear->next){
            break;
        }
    }
    return count;
}

//to be completed -omar

void update_queue_RR(process_queue* READY_QUEUE){
    if(get_count(READY_QUEUE)>1&&PCB_ENTRY.REMAINING_TIME-PCB_ENTRY.LAST_EXECUTED_TIME==TIME_QUANTUM){
        process_Node* temp=dequeue(READY_QUEUE);
        enqueue(READY_QUEUE,temp->Process);
    }
     if(PCB_ENTRY.REMAINING_TIME==0){
        PCB_ENTRY.FINISH_TIME=getClk();
        PCB_ENTRY.process_state=Finished;
        dequeue(READY_QUEUE);
        running_process_pid=-1;
    }
    if(get_count(READY_QUEUE)==1){
        return;
    }
}
int count_pid=-1;
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
    TIME_QUANTUM=atoi(argv[2]);// if RR 3
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    printf("queue id  is: %d\n",MESSAGE_ID);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    message_buf PROCESS_MESSAGE;
    process_count=0;
    /*---------------------------Omar Syed------------------------------------*/
    
        
            // printf("Scheduling Algorithm is Round Robin with Time Quantum = %d\n",TIME_QUANTUM);
            int firsttime =true; // TODO::fix this to be when queue is empty
            process_Node* current_process;
            while(1)
    {
                int rec_status = msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT);
                if(rec_status!=-1)
                {
                    // clock_timer = getClk();
                    // printf("clock now is: %d\n",clock_timer);
                    // printf("Process received with id %d & arritval time %d & priority %d and scheduling algorithm %d \n"
                    // ,PROCESS_MESSAGE.p.ID,PROCESS_MESSAGE.p.ARRIVAL_TIME,PROCESS_MESSAGE.p.PRIORITY,selected_Algorithm_NUM);
                    // PCB_ENTRY.p=PROCESS_MESSAGE.p;
                    // PCB_ENTRY.REMAINING_TIME=PROCESS_MESSAGE.p.RUNNING_TIME;
                    // PCB_ENTRY.RUNNING_TIME=0;
                    // PCB_ENTRY.START_TIME=-1;
                    // PCB_ENTRY.LAST_EXECUTED_TIME=-1;
                    // PCB_ENTRY.FINISH_TIME=-1;
                    // PCB_ENTRY.process_state=Ready;
                    // PCB_ENTRY.is_completed=false;
                    // enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
                    // process_count++;
                    // PRINT_READY_QUEUE();

                    switch (selected_Algorithm_NUM) {
                        case 1: 
                        {
                            //HPF
                            enqueue_priority(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);
                            PRINT_READY_PRIORITY_QUEUE();
                            break;
                        }
                        case 2:{
                            //SRTN
                            enqueue_priority_SRTN(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);
                            PRINT_READY_PRIORITY_QUEUE();
                            break;
                        }
                        case 3:
                        {
                            //RR
                            enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
                            PRINT_READY_QUEUE();
                              if(get_count(&READY_QUEUE)==1&&PROCESS_MESSAGE.p.first_time)
                                  {
                                      pid[++count_pid]=fork();
                                      if(pid[count_pid]==0){
                                          char str_remaining_time[10];
                                          sprintf(str_remaining_time, "%d", PROCESS_MESSAGE.p.RUNNING_TIME);
                                          execl("./process.out","./process.out", str_remaining_time, NULL);
                                          perror("Error in forking\n");
                                  }
                                  else
                                  {
                                    update_queue_RR(&READY_QUEUE);
                                  }
                                }
                                  else if (get_count(&READY_QUEUE)==1)
                                  {
                                    kill(pid[count_pid],SIGCONT);
                                    update_queue_RR(&READY_QUEUE);
                                  }
                                  else
                                  {
                                    //count_pid -> front  count_pid +1 -> front->next  count_pid-1 -> rear
                                    update_queue_RR(&READY_QUEUE);
                                         if(READY_QUEUE.front->Process.first_time){
                                             pid[++count_pid]=fork();
                                             if(pid[count_pid]==0){
                                               char str_remaining_time[10];
                                               sprintf(str_remaining_time, "%d", PROCESS_MESSAGE.p.RUNNING_TIME);
                                               execl("./process.out","./process.out", str_remaining_time, NULL);
                                               perror("Error in forking\n");
                                              }     
                                    }
                                    else{
                                        kill(++count_pid,SIGCONT);
                                    }
                                  }
                                
                            break;
                        }
                        default:
                            break;
                    
                    }
                }

                
                 if(clock_timer!=getClk())
                {
                    clock_timer=getClk();
                
                    printf("Clock Timer: %d\n",clock_timer);
                    
                    if (firsttime)
                    {
                        printf("started\n");
                        current_process = peek_front(&READY_QUEUE);
                        if(current_process != NULL)
                        {
                            printf("At time %d, Process P%d is running\n", clock_timer, current_process->Process.ID);
                            // TODO: Fork and execute the process here
                            // pid = fork();
                            // if (pid == 0) exec(...);
                            firsttime = false;
                        }
                    }
                                else if (clock_timer > 0 && clock_timer % TIME_QUANTUM == 0)
                        {  
                                            if(current_process != NULL)
                                            {
                                                // TODO: Stop current process
                                                // kill(current_process->Process.pid, SIGSTOP);
                                                
                                                current_process = current_process->next;
                                                if(current_process == NULL)
                                                {
                                                    current_process = READY_QUEUE.front;
                                                }
                                                
                                                if(current_process != NULL)
                                                {
                                                    printf("At time %d, Process P%d is running\n", clock_timer, current_process->Process.ID);
                                                    // TODO: Resume/start the process
                                                    // kill(current_process->Process.pid, SIGCONT);
                                                }
                                            }
                }

               
            

        }
    }
    destroyClk(true);

}
