#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>

/*---------------------------------Omar Syed------------------------------------*/
int MESSAGE_ID;
int finished_process=0;
int count_pid=-1;
PCB pcb[max];
FILE*pFile;
int * wait_time ;
float * WTA;
int count =0;
int total_running_time=0;

/*
1-Receive processes
2-Initialize Queues & PCB
3-Initialize Algorithms
4-RR
5-process.c
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

PCB* get_pcb(PCB*pcb,int process_count,int process_id){
    for(int i = 0 ;i<process_count;i++){
        if(pcb[i].process_id==process_id){
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

/*-----------------------Handler-----------------------*/
void handler(int signum){
    if(running_process_index >= 0 && running_process_index < process_count) {
        pcb[running_process_index].REMAINING_TIME = 0;
    }
}

/*---------------------------------Omar Syed------------------------------------*/
/*
Note :
    In  RR i assume if p2 arrived at the end of quanta of p1 i will execute p1 then p2 
*/

int main(int argc, char * argv[])
{
    signal(SIGUSR1, handler);

    //first line in Log File 
    pFile = fopen("scheduler.log", "a");
    if (!pFile) {
        printf("Error opening file.\n");
    } else {
        fprintf(pFile,"#At     time    x     process    y    state    arr    w    total    z    remain    wait    K\n");
        fclose(pFile);
    }

    int clock_timer= 0;
    int current_time =0;
    int total_process = atoi(argv[3]);
    wait_time = malloc(sizeof(int)*total_process);
    WTA = malloc(sizeof(float)*total_process);
    initClk();
    /*---------------------------Omar Syed------------------------------------*/

    //Inititalizations
    initialize_queue(&READY_QUEUE);
    initialize_priority_queue(&READY_PRIORITY_QUEUE);
    int selected_Algorithm_NUM=atoi(argv[1]);
    TIME_QUANTUM=atoi(argv[2]);// if RR 3
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    //printf("queue id  is: %d\n",MESSAGE_ID);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    message_buf PROCESS_MESSAGE;
    process_count=0;

    /*---------------------------Omar Syed------------------------------------*/

    while(1)
    {
        int rec_status = msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT);
        if(rec_status!=-1)
        {
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
                case 3:{
                    // RR
                    //PRINT_READY_QUEUE();
                    enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
                    //PRINT_READY_QUEUE();
                    
                    //Get Running Process Index to Access Pcb array
                    running_process_index=get_pcb_index(pcb,  process_count,peek_front(&READY_QUEUE)->Process.ID);

                    if(running_process_index == -1 && peek_front(&READY_QUEUE) != NULL && peek_front(&READY_QUEUE)->Process.first_time) {
                        //printf("\nInitialize pcb for first forking of process\n") ;
                        current_time = getClk();
                        peek_front(&READY_QUEUE)->Process.first_time = false;
                        
                        if(process_count >= max){
                            printf("Error: Max processes reached\n");
                            break;
                        }
                        
                        pcb[process_count].arrival_time = peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                        pcb[process_count].process_id = peek_front(&READY_QUEUE)->Process.ID;
                        pcb[process_count].RUNNING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                        pcb[process_count].REMAINING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                        pcb[process_count].START_TIME = getClk();
                        pcb[process_count].LAST_EXECUTED_TIME = getClk();
                        pcb[process_count].process_state = Running;
                        pcb[process_count].WAITING_TIME=0;
                        char str_rem_time[20];
                        sprintf(str_rem_time, "%d", peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
                        
                        //Run the process But this if it in the front of the queue 
                        int pid = fork();
                        if(pid == -1){
                            perror("Fork failed");
                            break;
                        }
                        if(pid == 0) {
                            execl("./process.out", "./process.out", str_rem_time, NULL);
                            perror("Error in execl");
                            exit(1);
                        }

                        pcb[process_count].process_pid = pid;
                        running_process_index = process_count;
                        process_count++;

                        //LOG file when start
                        pFile = fopen("scheduler.log", "a");
                        if(pFile) {
                            fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                                    current_time, pcb[running_process_index].process_id,
                                    pcb[running_process_index].arrival_time,
                                    pcb[running_process_index].RUNNING_TIME,
                                    pcb[running_process_index].REMAINING_TIME,
                                    current_time - pcb[running_process_index].arrival_time);
                            fclose(pFile);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if(selected_Algorithm_NUM==3 && clock_timer!=getClk() ){
            clock_timer=getClk();
            total_running_time++;
             process_Node* temp = READY_QUEUE.front;
        while(temp != NULL){
            int pcb_index = get_pcb_index(pcb, process_count, temp->Process.ID);
            if(pcb_index != -1 && pcb[pcb_index].process_state != Running) {
                pcb[pcb_index].WAITING_TIME++;
             }
            temp = temp->next;
            if(temp == READY_QUEUE.rear)
            break;
            }
            
            if(peek_front(&READY_QUEUE)==NULL) continue;
            //PRINT_READY_QUEUE();
            
            running_process_index=get_pcb_index(pcb,process_count,peek_front(&READY_QUEUE)->Process.ID);
            
            
            if(running_process_index==-1 && peek_front(&READY_QUEUE)->Process.first_time){
                //printf("\nInitialize pcb for first forking of process\n") ;
                current_time = getClk();
                peek_front(&READY_QUEUE)->Process.first_time = false;
            
                
                pcb[process_count].arrival_time = peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                pcb[process_count].process_id = peek_front(&READY_QUEUE)->Process.ID;
                pcb[process_count].RUNNING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                pcb[process_count].REMAINING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                pcb[process_count].START_TIME = getClk();
                pcb[process_count].LAST_EXECUTED_TIME = getClk();
                pcb[process_count].process_state = Running;
                pcb[process_count].WAITING_TIME= getClk()-pcb[process_count].arrival_time;
            
                char str_rem_time[20];
                sprintf(str_rem_time,"%d",peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
                int pid=fork();
                if(pid == -1){
                    perror("Fork failed");
                    continue;
                }
                if(pid==0){
                    execl("./process.out","./process.out",str_rem_time,NULL);
                    perror("Error in execl");
                    exit(1);
                }
                pcb[process_count].process_pid=pid;
                running_process_index=process_count;
                process_count++;
                
                pFile = fopen("scheduler.log", "a");
                if(pFile) {
                    fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                            getClk(), pcb[running_process_index].process_id,
                            pcb[running_process_index].arrival_time,
                            pcb[running_process_index].RUNNING_TIME,
                            pcb[running_process_index].REMAINING_TIME,
                            getClk() - pcb[running_process_index].arrival_time);
                    fclose(pFile);
                }
            } 
            
            if(running_process_index!=-1 && pcb[running_process_index].process_state == Running){
                pcb[running_process_index].REMAINING_TIME--;

                if(pcb[running_process_index].REMAINING_TIME <= 0){
                    int finished_id = pcb[running_process_index].process_id;
                    PCB* finished = &pcb[running_process_index];
                    
                    waitpid(finished->process_pid, NULL, 0);
                    finished->process_state = Finished;
                    finished->FINISH_TIME = getClk();
                    finished->is_completed = true;
                    
                    pFile = fopen("scheduler.log", "a");
                    if(pFile) {
                        fprintf(pFile, "At time %-5d process %-5d finished arr %-5d total %-5d remain %-5d wait %-5d TA %-5d WTA %.2f\n",
                                getClk(), finished->process_id,
                                finished->arrival_time, finished->RUNNING_TIME,
                                0,
                                getClk() - finished->arrival_time - finished->RUNNING_TIME,
                                finished->FINISH_TIME - finished->arrival_time,
                                (float)(finished->FINISH_TIME - finished->arrival_time) / finished->RUNNING_TIME);
                        fclose(pFile);
                    }
                    
                    WTA[count] = (float)(finished->FINISH_TIME - finished->arrival_time) / finished->RUNNING_TIME;
                    wait_time[count] = (getClk() - finished->arrival_time - finished->RUNNING_TIME);
                    count++;
                    
                    process_Node* temp = dequeue(&READY_QUEUE);
                    remove_pcb(pcb, &process_count, finished_id);
                    finished_process++;
                    
                    if(peek_front(&READY_QUEUE) != NULL) {
                        running_process_index = get_pcb_index(pcb, process_count, peek_front(&READY_QUEUE)->Process.ID);
                        
                        if(running_process_index != -1 && running_process_index < process_count) {
                            kill(pcb[running_process_index].process_pid, SIGCONT);
                            pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
                            pcb[running_process_index].process_state = Running;
                            
                            pFile = fopen("scheduler.log", "a");
                            if(pFile) {
                                fprintf(pFile, "At time %-5d process %-5d resumed arr %-5d total %-5d remain %-5d wait %-5d\n",
                                        getClk(), pcb[running_process_index].process_id,
                                        pcb[running_process_index].arrival_time,
                                        pcb[running_process_index].RUNNING_TIME,
                                        pcb[running_process_index].REMAINING_TIME,
                                        getClk() - pcb[running_process_index].arrival_time - 
                                        (pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME));
                                fclose(pFile);
                            }
                        } else if(running_process_index == -1 && peek_front(&READY_QUEUE)->Process.first_time) {
                            
                            peek_front(&READY_QUEUE)->Process.first_time = false;
                            
                            pcb[process_count].process_state = Running;
                            pcb[process_count].process_id = peek_front(&READY_QUEUE)->Process.ID;
                            pcb[process_count].RUNNING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                            pcb[process_count].arrival_time = peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                            pcb[process_count].REMAINING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                            pcb[process_count].START_TIME = getClk();
                            pcb[process_count].LAST_EXECUTED_TIME = getClk();
                            
                            char str_rem_time[20];
                            sprintf(str_rem_time, "%d", peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
                            int pid = fork();
                            if(pid == 0){
                                execl("./process.out", "./process.out", str_rem_time, NULL);
                                perror("Error in execl\n");
                                exit(1);
                            }
                            pcb[process_count].process_pid = pid;
                            running_process_index = process_count;
                            process_count++;
                            
                            pFile = fopen("scheduler.log", "a");
                            if(pFile) {
                                fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n", 
                                        getClk(), pcb[running_process_index].process_id,
                                        pcb[running_process_index].arrival_time,
                                        pcb[running_process_index].RUNNING_TIME, 
                                        pcb[running_process_index].REMAINING_TIME,
                                        getClk() - pcb[running_process_index].arrival_time);
                                fclose(pFile);
                            }
                        }
                    } else {
                        running_process_index = -1;
                    }
                    continue;
                }

                int time_executed=getClk()-pcb[running_process_index].LAST_EXECUTED_TIME;
                    
                if(time_executed>=TIME_QUANTUM ){
                    kill(pcb[running_process_index].process_pid,SIGSTOP);
                    pcb[running_process_index].process_state=Ready;
                    pcb[running_process_index].LAST_EXECUTED_TIME=getClk();
                    
                    pFile = fopen("scheduler.log", "a");
                    if(pFile) {
                        fprintf(pFile, "At time %-5d process %-5d stopped arr %-5d total %-5d remain %-5d wait %-5d\n",
                                getClk(), pcb[running_process_index].process_id,
                                pcb[running_process_index].arrival_time,
                                pcb[running_process_index].RUNNING_TIME,
                                pcb[running_process_index].REMAINING_TIME,
                                getClk() - pcb[running_process_index].arrival_time - 
                                (pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME));
                        fclose(pFile);
                    }
                    
                    process_Node* temp=dequeue(&READY_QUEUE);
                    enqueue(&READY_QUEUE,temp->Process);

                    running_process_index = get_pcb_index(pcb, process_count, peek_front(&READY_QUEUE)->Process.ID);
                    
                    if(running_process_index == -1 && peek_front(&READY_QUEUE)->Process.first_time){
                        peek_front(&READY_QUEUE)->Process.first_time=false;
                        
                       
                        pcb[process_count].arrival_time=peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                        pcb[process_count].process_id=peek_front(&READY_QUEUE)->Process.ID;
                        pcb[process_count].RUNNING_TIME=peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                        pcb[process_count].REMAINING_TIME=peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                        pcb[process_count].START_TIME=getClk();
                        pcb[process_count].LAST_EXECUTED_TIME=getClk();
                        pcb[process_count].process_state=Running;
                        
                        char str_rem_time[20];
                        sprintf(str_rem_time,"%d",peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
                        int pid=fork();
                        if(pid == -1){
                            perror("Fork failed");
                            continue;
                        }
                        if(pid==0){
                            execl("./process.out","./process.out",str_rem_time,NULL);
                            perror("Error in execl");
                            exit(1);
                        }
                        pcb[process_count].process_pid=pid;
                        running_process_index=process_count;
                        process_count++;
                        
                        pFile = fopen("scheduler.log", "a");
                        if(pFile) {
                            fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                                    getClk(), pcb[running_process_index].process_id,
                                    pcb[running_process_index].arrival_time,
                                    pcb[running_process_index].RUNNING_TIME,
                                    pcb[running_process_index].REMAINING_TIME,
                                    getClk() - pcb[running_process_index].arrival_time - 
                                    (pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME));
                            fclose(pFile);
                        }
                    } 
                    else if(running_process_index!= -1 && pcb[running_process_index].REMAINING_TIME > 0){
                        kill(pcb[running_process_index].process_pid,SIGCONT);
                        pcb[running_process_index].LAST_EXECUTED_TIME=getClk();
                        pcb[running_process_index].process_state=Running;
                        
                        pFile = fopen("scheduler.log", "a");
                        if(pFile) {
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

        // Generate performance file
        if(finished_process==total_process){
    int AVGWAITING=0;
    float AVGWTA = 0;
    for(int i=0;i<count;i++){
        AVGWTA+=WTA[i];
        AVGWAITING+=wait_time[i];
    }

    pFile=fopen("scheduler.perf", "w");
    if(pFile) {
        fprintf(pFile, "Cpu utilization = %-4f %% \n Avg WTA = %-4f \n Avg Waiting = %-4f \n ", 
                ((float)total_running_time/getClk())*100, 
                AVGWTA/total_process, 
                (float)AVGWAITING/total_process);
        fclose(pFile);
        printf("\nPerformance File Has Been Generated !\a\n");
    }
    finished_process=0;
    total_process=-1;
}
    }

    
    
    destroyClk(true);
}