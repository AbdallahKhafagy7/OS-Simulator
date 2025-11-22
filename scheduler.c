#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "headers.h"
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
process_queue READY_QUEUE;
process_priority_queue READY_PRIORITY_QUEUE;
int TIME_QUANTUM;
int pid[max];
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

int count_pid=-1;
PCB pcb[max];
FILE*pFile;

PCB* get_pcb(PCB*pcb,int process_count,int process_id){
    for(int i = 0 ;i<process_count;i++){
        if(pcb[i].process_id==process_id){
            return &pcb[i];
        }
    }
    return NULL;
}

PCB* get_pcb_pid(PCB*pcb,int process_count,int process_pid){
    for(int i = 0 ;i<process_count;i++){
        if(pcb[i].process_pid==process_pid){
            return &pcb[i];
        }
    }
    return NULL;
}

int get_pcb_index(PCB*pcb,int process_count,int process_id){
    for(int i = 0 ;i<process_count;i++){
        if(pcb[i].process_id==process_id){
            return i;
        }
    }
    return -1;
}

void remove_pcb(PCB*pcb,int *process_count,int process_id){
    int k= get_pcb_index(pcb,  *process_count,  process_id);
    for(int i=k;i<*process_count-1;i++){
        pcb[i]=pcb[i+1];
    }
    (*process_count)--;
}

void update_queue_RR(process_queue* READY_QUEUE){
    int index = get_pcb_index(pcb, process_count, peek_front(READY_QUEUE)->Process.ID);
    int current_time =getClk();
    if(current_time-pcb[index].LAST_EXECUTED_TIME>=TIME_QUANTUM){
        pcb[index].process_state=Ready;
         kill(pcb[index].process_pid, SIGSTOP);
        printf("Stopped\n");
        process_Node* temp =dequeue(READY_QUEUE);
        PRINT_READY_QUEUE();
        enqueue(READY_QUEUE, temp->Process);
        PRINT_READY_QUEUE();
    }
    else{
        pcb[index].REMAINING_TIME--;
    }
}


void handler(int signum){
    printf("Handler called - Process finished\n");
    
    if(peek_front(&READY_QUEUE) == NULL) {
        printf("Error: Queue is empty in handler\n");
        return;
    }
    
    int finished_id = peek_front(&READY_QUEUE)->Process.ID;
    PCB* finished = get_pcb(pcb, process_count, finished_id);
    
    if(finished == NULL) {
        printf("Error: Could not find PCB for process %d\n", finished_id);
        return;
    }
    
    waitpid(finished->process_pid, NULL, 0);
    finished->process_state = Finished;
    finished->FINISH_TIME = getClk();
    finished->is_completed = true;
    finished->REMAINING_TIME = 0;
     pFile = fopen("scheduler_log.txt", "a");
    if (!pFile) {
        printf("Error opening file.\n");
    }
        fprintf(pFile, "At time %-5d process %-5d finished arr %-5d total %-5d remain %-5d wait %-5d TA %-5d WTA %.2f\n",
            finished->FINISH_TIME, finished->process_id,finished->arrival_time,finished->RUNNING_TIME,
            finished->REMAINING_TIME, finished->FINISH_TIME - finished->arrival_time - finished->RUNNING_TIME,finished->FINISH_TIME - finished->arrival_time,
            (float)(finished->FINISH_TIME - finished->arrival_time) / finished->RUNNING_TIME);
    fclose(pFile);
    remove_pcb(pcb, &process_count, finished_id);
    dequeue(&READY_QUEUE);
    printf("Process %d finished and removed from queue\n", finished_id);
    PRINT_READY_QUEUE();
    if(peek_front(&READY_QUEUE)->Process.first_time!=true){
        running_process_index=get_pcb_index(pcb,process_count,peek_front(&READY_QUEUE)->Process.ID);
        if(running_process_index != -1) {
            kill(pcb[running_process_index].process_pid, SIGCONT);
            pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
            pcb[running_process_index].process_state = Running;
            printf("Resumed process %d\n", pcb[running_process_index].process_id);
            pFile = fopen("scheduler_log.txt", "a");
             fprintf(pFile, "At time %-5d process %-5d resumed arr %-5d total %-5d remain %-5d wait %-5d\n", getClk(), pcb[running_process_index].process_id,
                    pcb[running_process_index].arrival_time, pcb[running_process_index].RUNNING_TIME, pcb[running_process_index].REMAINING_TIME,getClk() - pcb[running_process_index].arrival_time - (pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME));
                    fclose(pFile);
        }
    }
    else{
        pcb[process_count].process_state = Running;
        pcb[process_count].process_id = peek_front(&READY_QUEUE)->Process.ID;
        pcb[process_count].RUNNING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
        pcb[process_count].arrival_time = peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
        pcb[process_count].REMAINING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
        pcb[process_count].START_TIME = getClk();
        pcb[process_count].LAST_EXECUTED_TIME = getClk();
        char str_rem_time [ 20];
        sprintf(str_rem_time, "%d",peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
        int pid=fork();
        if(pid==0){
        execl("./process.out","./process.out", str_rem_time,NULL);
        perror("Error in execl\n");
        exit(1);
        }
        else{
            pcb[process_count].process_pid = pid;
            peek_front(&READY_QUEUE)->Process.first_time = false;
            running_process_index = process_count;
            process_count++;
            printf("Started new process %d\n", pcb[running_process_index].process_id);
            pFile=fopen("scheduler_log.txt", "a");
            fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n", getClk(), pcb[running_process_index].process_id,pcb[running_process_index].arrival_time,
        pcb[running_process_index].RUNNING_TIME, pcb[running_process_index].REMAINING_TIME,getClk() - pcb[running_process_index].arrival_time);
        fclose(pFile);
        }
    }
}

/*---------------------------------Omar Syed------------------------------------*/
int main(int argc, char * argv[])
{
    signal(SIGUSR1, handler);
    pFile = fopen("scheduler_log.txt", "a");
    if (!pFile) {
        printf("Error opening file.\n");
    }
     fprintf(pFile, "%-5s %-10s %-10s %-10s %-20s %-5s %-10s %-10s %-10s %-20s %-10s %-20s\n",
        "#At", "time", "x", "process", "y","state","arr","w","total","z","remain","wait");
        fclose(pFile);
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

                   switch(selected_Algorithm_NUM) {
                case 1:
                    // HPF
                    enqueue_priority(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);
                    PRINT_READY_PRIORITY_QUEUE();
                    break;
                case 2:
                    // SRTN
                    enqueue_priority_SRTN(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);
                    PRINT_READY_PRIORITY_QUEUE();
                    break;
                case 3:
                    // RR
                    enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
                    PRINT_READY_QUEUE();
                    break;
                default:
                    break;
            }

        }

        if(selected_Algorithm_NUM==3){
             if(clock_timer!=getClk()){

                 if (peek_front(&READY_QUEUE) == NULL) {
                     continue;
                 }
                        clock_timer=getClk();
                        printf("Clock Timer: %d\n",clock_timer);
                        PRINT_READY_QUEUE();   
                if(get_count(&READY_QUEUE)==1 && peek_front(&READY_QUEUE)->Process.first_time==true){
                    peek_front(&READY_QUEUE)->Process.first_time=false;
                     pcb[process_count].arrival_time = peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                        pcb[process_count].process_id = peek_front(&READY_QUEUE)->Process.ID;
                        pcb[process_count].RUNNING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                        pcb[process_count].REMAINING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                        pcb[process_count].START_TIME = getClk();
                        pcb[process_count].LAST_EXECUTED_TIME = getClk();
                        pcb[process_count].process_state = Running;               
                        char str_rem_time [ 20];
                    sprintf(str_rem_time, "%d",peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
                    int pid =fork();
                    if(pid==0){
                        execl("./process.out","./process.out",str_rem_time,NULL);
                        perror("Error in forking\n");
                    }
                    else{
                        pcb[process_count].process_pid = pid;
                         peek_front(&READY_QUEUE)->Process.first_time = false;
                         running_process_index = process_count;
                         process_count++;
                         pFile=fopen("scheduler_log.txt", "a");
                         fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                                 getClk(), pcb[running_process_index].process_id,
                                 pcb[running_process_index].arrival_time,
                                 pcb[running_process_index].RUNNING_TIME,
                                 pcb[running_process_index].REMAINING_TIME,
                                 0);
                         fclose(pFile);
                       }
                       continue;
                }
                    if(peek_front(&READY_QUEUE) != NULL){
                        running_process_index = get_pcb_index(pcb,  process_count,peek_front(&READY_QUEUE)->Process.ID);
                        if(running_process_index==-1)continue;
                   int time_executed = getClk() - pcb[running_process_index].LAST_EXECUTED_TIME;
                    pcb[running_process_index].REMAINING_TIME =pcb[running_process_index].REMAINING_TIME- time_executed;
                    if(time_executed>=TIME_QUANTUM && get_count(&READY_QUEUE)>1){
                         pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
                        kill(pcb[running_process_index].process_pid, SIGSTOP);
                        pcb[running_process_index].process_state = Ready;   

                          pFile=fopen("scheduler_log.txt", "a");
                         fprintf(pFile, "At time %-5d process %-5d stopped arr %-5d total %-5d remain %-5d wait %-5d\n",
                       getClk(), pcb[running_process_index].process_id,
                       pcb[running_process_index].arrival_time,
                       pcb[running_process_index].RUNNING_TIME,
                       pcb[running_process_index].REMAINING_TIME,
                       getClk() - pcb[running_process_index].arrival_time - 
                       (pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME));
                         fclose(pFile);
            


                        printf("Stopped\n");
                        process_Node* temp =dequeue(&READY_QUEUE);
                        PRINT_READY_QUEUE();
                        enqueue(&READY_QUEUE, temp->Process);
                        PRINT_READY_QUEUE();
                        if(peek_front(&READY_QUEUE)->Process.first_time==true){
                             pcb[process_count].arrival_time = peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                             pcb[process_count].process_id = peek_front(&READY_QUEUE)->Process.ID;
                             pcb[process_count].RUNNING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                             pcb[process_count].REMAINING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                             pcb[process_count].START_TIME = getClk();
                             pcb[process_count].LAST_EXECUTED_TIME = getClk();
                             pcb[process_count].process_state = Running;
                        char str_rem_time [ 20];
                        sprintf(str_rem_time, "%d",peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
                        printf("Forked\n");
                        PRINT_READY_QUEUE();
                        int pid =fork();
                          if(pid==0){
                              execl("./process.out","./process.out",str_rem_time,NULL);
                              perror("Error in forking\n");
                            }
                          else{
                            PRINT_READY_QUEUE();
                            pcb[process_count].process_pid = pid;
                            peek_front(&READY_QUEUE)->Process.first_time = false;
                            running_process_index = process_count;
                            process_count++;
                            pFile = fopen("scheduler_log.txt", "a");
                             fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                            getClk(), pcb[running_process_index].process_id,
                            pcb[running_process_index].arrival_time,
                            pcb[running_process_index].RUNNING_TIME,
                            pcb[running_process_index].REMAINING_TIME,
                            getClk() - pcb[running_process_index].arrival_time);
                            fclose(pFile);
                          }
                        }
                        else{
                            running_process_index=get_pcb_index(pcb,  process_count, peek_front(&READY_QUEUE)->Process.ID);
                            if(running_process_index!=-1){
                            //printf("Cont %d \n",running_process_index);
                            pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
                            pcb[running_process_index].process_state = Running;
                            kill(pcb[running_process_index].process_pid, SIGCONT);
                             pFile = fopen("scheduler_log.txt", "a");
                             fprintf(pFile, "At time %-5d process %-5d resumed arr %-5d total %-5d remain %-5d wait %-5d\n",
                            getClk(), pcb[running_process_index].process_id,
                            pcb[running_process_index].arrival_time,
                            pcb[running_process_index].RUNNING_TIME,
                            pcb[running_process_index].REMAINING_TIME,
                            getClk() - pcb[running_process_index].arrival_time - 
                            (pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME));
                            fclose(pFile);
                        
                        }

                        }
                        
                    }
                    
                }
            }
        }
        
            }
            destroyClk(true);
            }
            
                
        //          if(clock_timer!=getClk())
        //         {
        //             clock_timer=getClk();
                
        //             printf("Clock Timer: %d\n",clock_timer);


        // }