#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "headers.h"
#include <math.h>
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
float std_dev_sqr=0;
int count =0;
int total_running_time=0;
 int selected_Algorithm_NUM=-1;
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
    if(selected_Algorithm_NUM==3){
    if(running_process_index >= 0 && running_process_index < process_count) {
        pcb[running_process_index].REMAINING_TIME = 0;
    }}
    else if(selected_Algorithm_NUM==2){
          printf("Handler called - Process finished\n");

    int finished_id = -1;

    // If SRTN is active (Priority Queue has data)
   // 1. GET ID FROM PCB (The one that actually finished)
    if (running_process_index == -1) return; // Safety check
     finished_id = pcb[running_process_index].process_id;

    // 2. SEARCH AND REMOVE (SRTN Specific)
    
        if (READY_PRIORITY_QUEUE.front != NULL) {
            process_PNode* current = READY_PRIORITY_QUEUE.front;
            process_PNode* prev = NULL;
            
            // Loop to find the specific ID (Process 3)
            while (current != NULL) {
                if (current->Process.ID == finished_id) {
                    // Unlink (Remove) the node
                    if (prev == NULL) READY_PRIORITY_QUEUE.front = current->next;
                    else prev->next = current->next;
                    
                    free(current); // Delete it
                    break; 
                }
                prev = current;
                current = current->next;
            }
        }
    
   
    else {
        printf("Error: Queue is empty but handler was called\n");
        return;
    }

    // --- STANDARD PCB CLEANUP ---
    PCB* finished = get_pcb(pcb, process_count, finished_id);
    
    if(finished == NULL) {
        printf("Error: Could not find PCB for process %d\n", finished_id);
        return;
    }
    
    // Wait ensures the process is fully removed from OS process table
    waitpid(finished->process_pid, NULL, 0);

    // Update Process State
    finished->process_state = Finished;
    finished->FINISH_TIME = getClk();
    finished->is_completed = true;
    finished->REMAINING_TIME = 0;

    // Logging
    pFile = fopen("scheduler.log", "a");
    if (pFile) {
        int TA = finished->FINISH_TIME - finished->arrival_time;
        //int WT = TA - finished->RUNNING_TIME;
        int wait = finished->FINISH_TIME - finished->arrival_time - finished->RUNNING_TIME;
        float WTA = (float)TA / finished->RUNNING_TIME;
        
        fprintf(pFile, "At time %-5d process %-5d finished arr %-5d total %-5d remain %-5d wait %-5d TA %-5d WTA %.2f\n",
            finished->FINISH_TIME, finished->process_id, finished->arrival_time, 
            finished->RUNNING_TIME, 0, wait, TA, WTA);
        fclose(pFile);
    }

    remove_pcb(pcb, &process_count, finished_id);
    
    
    printf("Process %d finished and removed from queue\n", finished_id);

  
    running_process_index = -1; 
        
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
    bool new_process= false;
    bool time_moved= false;
    initClk();
    /*---------------------------Omar Syed------------------------------------*/

    //Inititalizations
    initialize_queue(&READY_QUEUE);
    initialize_priority_queue(&READY_PRIORITY_QUEUE);
     selected_Algorithm_NUM=atoi(argv[1]);
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
                    new_process=true;
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



         if(selected_Algorithm_NUM == 2) {
        
   

    if(clock_timer != getClk()|| new_process) {
       
        if (clock_timer != getClk()) {
             clock_timer = getClk();
            time_moved = true;
        }
        new_process=false;
        printf("Clock Timer: %d\n", clock_timer);
        if (clock_timer ==0)
        printf("Scheduling Algorithm is SRTN\n");

        
        if (peek_priority_front(&READY_PRIORITY_QUEUE) == NULL) {
            continue; // forces to next iteration of while loop if queue is empty 
        }

        // Identify shortest ID and Currently Running ID
        int shortest_id = peek_priority_front(&READY_PRIORITY_QUEUE)->ID;
        int current_running_id = -1;

        if (running_process_index != -1) {
            current_running_id = pcb[running_process_index].process_id;
        }

          if (time_moved) {
            
        
        if (current_running_id != -1 && current_running_id == shortest_id) //  process running and running is the shortest
        {
           
            
        
            if (running_process_index == -1) {
                running_process_index = get_pcb_index(pcb, process_count, shortest_id);
                // nothing was running so set the index to shortest
            }

            // Decrement Time id there is a running process and it has remaining time
            if (running_process_index != -1 && pcb[running_process_index].REMAINING_TIME > 0) {
                pcb[running_process_index].REMAINING_TIME--;
                if (peek_priority_front(&READY_PRIORITY_QUEUE) != NULL)
                peek_priority_front(&READY_PRIORITY_QUEUE)->RUNNING_TIME=pcb[running_process_index].REMAINING_TIME; //change running time so queue understand
            }
              
        }
        } 
        if (current_running_id == -1 || current_running_id != shortest_id) 
        
        {
            // not shortest running (preemption needed)

            //  stop signal to current running process
            if (running_process_index != -1) {
                kill(pcb[running_process_index].process_pid, SIGSTOP);
                pcb[running_process_index].process_state = Ready;
                pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
                printf("Preempted process %d\n", pcb[running_process_index].process_id);
                
                // Log Stop
                pFile = fopen("scheduler.log", "a");
                fprintf(pFile, "At time %-5d process %-5d stopped arr %-5d total %-5d remain %-5d wait %-5d\n",
                    getClk(), pcb[running_process_index].process_id,
                    pcb[running_process_index].arrival_time, pcb[running_process_index].RUNNING_TIME,
                    pcb[running_process_index].REMAINING_TIME,
                    getClk() - pcb[running_process_index].arrival_time - (pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME));
                fclose(pFile);
            }

            // B. Start/Resume the new shortest Process
            process* shortest_process_node = peek_priority_front(&READY_PRIORITY_QUEUE);

            if (shortest_process_node->first_time) {
                // Frist time process needs forking and initialization
                pcb[process_count].process_id = shortest_process_node->ID;
                pcb[process_count].arrival_time = shortest_process_node->ARRIVAL_TIME;
                pcb[process_count].RUNNING_TIME = shortest_process_node->RUNNING_TIME;
                pcb[process_count].REMAINING_TIME = shortest_process_node->RUNNING_TIME;
                pcb[process_count].START_TIME = getClk();
                pcb[process_count].process_state = Running;
                
                char str_rem_time[20];
                sprintf(str_rem_time, "%d", pcb[process_count].RUNNING_TIME);
                
                int pid = fork();
                if (pid == 0) {
                    execl("./process.out", "./process.out", str_rem_time, NULL);
                    exit(0);
                } else {
                    pcb[process_count].process_pid = pid;
                    shortest_process_node->first_time = false;
                    running_process_index = process_count;
                    process_count++;
                    
                    printf("Started new process %d\n", pcb[running_process_index].process_id);
                    
                    // Log Start
                    pFile = fopen("scheduler.log", "a");
                    fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                        getClk(), pcb[running_process_index].process_id,
                        pcb[running_process_index].arrival_time, pcb[running_process_index].RUNNING_TIME,
                        pcb[running_process_index].REMAINING_TIME,
                        getClk() - pcb[running_process_index].arrival_time);
                    fclose(pFile);
                }
            } else {
                // RESUME EXISTING PROCESS NO FORKING
                running_process_index = get_pcb_index(pcb, process_count, shortest_process_node->ID);
                if (running_process_index != -1) {
                    kill(pcb[running_process_index].process_pid, SIGCONT);
                    pcb[running_process_index].process_state = Running;
                    
                    printf("Resumed process %d\n", pcb[running_process_index].process_id);
                    
                    // Log Resume
                    pFile = fopen("scheduler.log", "a");
                    fprintf(pFile, "At time %-5d process %-5d resumed arr %-5d total %-5d remain %-5d wait %-5d\n",
                        getClk(), pcb[running_process_index].process_id,
                        pcb[running_process_index].arrival_time, pcb[running_process_index].RUNNING_TIME,
                        pcb[running_process_index].REMAINING_TIME,
                        getClk() - pcb[running_process_index].arrival_time - (pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME));
                    fclose(pFile);
                }
            }
        }
    }
}





        if(selected_Algorithm_NUM==3 && clock_timer!=getClk() ){
            clock_timer=getClk();
            printf("Clock Timer : %d \n",clock_timer);
            if(running_process_index!=-1&&pcb[running_process_index].process_state == Running){
                total_running_time++;
            }
             process_Node* temp = READY_QUEUE.front;
        while(temp != NULL){
            int index = get_pcb_index(pcb, process_count, temp->Process.ID);
            if(index != -1 && pcb[index].process_state != Running) {
                pcb[index].WAITING_TIME++;
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
                               int total_executed = pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME;
                                int wait = getClk() - pcb[running_process_index].arrival_time - total_executed;
                                fprintf(pFile, "At time %-5d process %-5d resumed arr %-5d total %-5d remain %-5d wait %-5d\n",
                                        getClk(), pcb[running_process_index].process_id,
                                        pcb[running_process_index].arrival_time,
                                        pcb[running_process_index].RUNNING_TIME,
                                        pcb[running_process_index].REMAINING_TIME,
                                        wait);
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
    }

        // Generate performance file
        if(finished_process==total_process){
    int AVGWAITING=0;
    float AVGWTA = 0;
    for(int i=0;i<count;i++){
        AVGWTA+=WTA[i];
        AVGWAITING+=wait_time[i];
    }

    for(int i=0;i<count;i++){
        std_dev_sqr+=pow((WTA[i]-AVGWTA),2);
    }
    std_dev_sqr=std_dev_sqr/total_process;
    float std_dev = sqrt(std_dev_sqr);

    pFile=fopen("scheduler.perf", "w");
    if(pFile) {
        fprintf(pFile, "Cpu utilization = %-4f %% \nAvg WTA = %-4f \nAvg Waiting = %-4f \nstd WTA = %-4f \n ", 
                ((float)total_running_time/getClk())*100, 
                AVGWTA/total_process, 
                (float)AVGWAITING/total_process,std_dev );
        fclose(pFile);
        printf("\nPerformance File Has Been Generated !\a\n");
    }
    finished_process=0;
    total_process=-1;
}
    

    
    return 0;
    destroyClk(true);
}