#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "headers.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include "mmu.h"
/*---------------------------------Omar Syed Final------------------------------------*/
int MESSAGE_ID;
//int MESSAGE_sch_ID;
int scheduler_process = 0;
int finished_process=0;
int count_pid=-1;
PCB pcb[max];
FILE*pFile;
int * wait_time ;
int * total_running_time ;
int total_running_times=0;
float * WTA;
int running_count=0;
float std_dev_sqr=0;
int count =0;
int selected_Algorithm_NUM=-1;
process_queue READY_QUEUE;
process_queue BLOCKED_QUEUE;
process_priority_queue READY_PRIORITY_QUEUE;
int TIME_QUANTUM;
int current_time = 0;
int pid[max];
int running_process_index=-1;
int process_count=0;
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






// hpf functions
PCB* pcbArray[max];
int pcbCount = 0;
PCB* runningPcb = NULL;
PcbPriorityQueue readyPriorityQueue;
int timer = 0;
int childFinished = 0;

PCB* getPcbById(int processId) {
    for (int i = 0; i < pcbCount; i++)
        if (pcbArray[i]->process_id == processId)
            return pcbArray[i];
    return NULL;
}

PCB* getPcbByPid(int pid) {
    for (int i = 0; i < pcbCount; i++){
        if (pcbArray[i]->process_pid == pid){ /// problem is most probably here
            return pcbArray[i];
        }
    }
    return NULL;
}

void printPriorityQueue(PcbPriorityQueue* queue) {
    if (!queue || isPriorityQueueEmpty(queue)) {
        printf("Priority Queue is empty.\n");
        return;
    }

    printf("Priority Queue (front -> rear):\n");
    PcbNode* current = queue->front;
    while (current) {
        PCB* p = current->pcb;
        printf("P%d [prio=%d, dep=%d, remaining=%d] -> ",
               p->process_id, p->priority, p->dependency_id, p->REMAINING_TIME);
        current = current->next;
    }
    printf("NULL\n");
}

void printPCB(PCB* p) {
    if (!p) {
        printf("PCB = NULL\n");
        return;
    }

    printf("--------------------------------------------------\n");
    printf("PCB for process %d\n", p->process_id);
    printf("--------------------------------------------------\n");

    printf("Process PID        : %d\n", p->process_pid);
    printf("Priority           : %d\n", p->priority);
    printf("Dependency ID      : %d\n", p->dependency_id);

    printf("State              : ");
    switch (p->process_state) {
        case Ready:    printf("READY\n"); break;
        case Running:  printf("RUNNING\n"); break;
        case Finished: printf("FINISHED\n"); break;
        default:       printf("UNKNOWN\n"); break;
    }

    printf("Started?           : %s\n", p->STARTED ? "YES" : "NO");
    printf("Completed?         : %s\n", p->is_completed ? "YES" : "NO");

    printf("Arrival Time       : %d\n", p->arrival_time);
    printf("Start Time         : %d\n", p->START_TIME);
    printf("Last Exec Time     : %d\n", p->LAST_EXECUTED_TIME);
    printf("Finish Time        : %d\n", p->FINISH_TIME);

    printf("Requested Runtime  : %d\n", p->RUNNING_TIME);
    printf("Remaining Time     : %d\n", p->REMAINING_TIME);

    printf("--------------------------------------------------\n\n");
}

void printLog(PCB* p, char *string) 
{
    pFile = fopen("scheduler_log.txt", "a");

    if (strcmp(string, "finished") == 0)
    {
        int TA = p->FINISH_TIME - p->arrival_time;
        float WTA = (float)TA / p->RUNNING_TIME;
        int W = TA - p->RUNNING_TIME;

        fprintf(pFile,
                "At time %d process %d %s arrival %d total %d remain %d wait %d TA %d WTA %.2f\n",
                getClk(),
                p->process_id,
                string,
                p->arrival_time,
                p->RUNNING_TIME,
                p->REMAINING_TIME,
                W,
                TA,
                WTA
        );
    }
    else
    {
        fprintf(pFile,
                "At time %d process %d %s arrival %d total %d remain %d\n",
            getClk(),
                p->process_id,
                string,
                p->arrival_time,
                p->RUNNING_TIME,
                p->REMAINING_TIME
            );
    }

    fclose(pFile);
}

void runPcb(PCB* p, int currentTick) {
    if (!p->STARTED) {
        int pid = fork();
        if (pid == 0) { // child
            char timeArg[16];
            sprintf(timeArg, "%d", p->REMAINING_TIME);
            execl("./process2.out", "./process2.out", timeArg, NULL);
            exit(1);
        }

        // Update PCB
        p->STARTED = true;
        p->process_pid = pid;
        p->process_state = Running;
        p->START_TIME = currentTick;
        p->LAST_EXECUTED_TIME = currentTick;

        printLog(p, "started");
    } else {
        // Resume process
        kill(p->process_pid, SIGCONT);
        p->process_state = Running;
        p->LAST_EXECUTED_TIME = currentTick;

        printLog(p, "resumed");
    }
    printPCB(p);
}

bool depCheck(PCB* p) {
    // Case 1: No dependency
    if (p->dependency_id == -1)
        return true;

    // Get the PCB of the dependency
    PCB* depPcb = getPcbById(p->dependency_id);

    // Case 2: Dependency not arrived yet
    if (depPcb == NULL)
        return false;

    // Case 3: Dependency arrived but not finished
    if (depPcb->is_completed == false)
        return false;

    // Case 4: Dependency finished
    return true;
}

void hpfLoop(int currentTick) {
    timer = currentTick;

    // PREEMPTION CHECK
    if (runningPcb != NULL && !isPriorityQueueEmpty(&readyPriorityQueue)) {
        PCB* top = peekPriorityFront(&readyPriorityQueue);

        if (top->priority < runningPcb->priority) {
            kill(runningPcb->process_pid, SIGSTOP);
            enqueuePriority(&readyPriorityQueue, runningPcb);
            printLog(runningPcb, "stopped");
            runningPcb = NULL;
        }
    }

    // Picking next process (dependency-safe, no infinite loop)
    if (runningPcb == NULL) {
        PCB* dep[1000];
        int depCount = 0;

        PCB* candidate = NULL;

        while (!isPriorityQueueEmpty(&readyPriorityQueue)) {
            PCB* p = dequeuePriority(&readyPriorityQueue);

            if (depCheck(p)) {
                candidate = p;
                break;
            } else {
                dep[depCount++] = p;
            }
        }

        for (int i = 0; i < depCount; i++)
            enqueuePriority(&readyPriorityQueue, dep[i]);

        if (candidate != NULL) {
            runningPcb = candidate;
            runPcb(runningPcb, timer);
        }
    }

    // UPDATE REMAINING TIME
    if (runningPcb != NULL && runningPcb->REMAINING_TIME > 0) {
        runningPcb->REMAINING_TIME--;
        runningPcb->LAST_EXECUTED_TIME = timer;
    }
}

void myHandler(int signum){
    int status;
    int finishedPid = wait(&status);
    PCB* finished = getPcbByPid(finishedPid);
    if (!finished) return;

    int now = timer; // use the tick snapshot
    finished->FINISH_TIME = now;
    finished->REMAINING_TIME = 0;
    finished->process_state = Finished;
    finished->is_completed = true;
    printf("now is:%d, finishis:%d\n", now, finished->FINISH_TIME);

    printLog(finished, "finished");

    runningPcb = NULL;

    // Update dependent processes
    for (int i = 0; i < pcbCount; i++) {
        if (pcbArray[i]->dependency_id == finished->process_id)
            pcbArray[i]->dependency_id = -1;
    }

    childFinished = 1; // main loop will handle next scheduling
    printPCB(finished);
}

/*---------------------------------Omar Syed------------------------------------*/


int main(int argc, char * argv[])
{
    //first line in Log File 
    pFile = fopen("scheduler.log", "w");
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
    total_running_time = malloc(sizeof(float)*total_process);
    bool new_process= false;
    bool time_moved= false;
    initClk();
    /*---------------------------Omar Syed------------------------------------*/

    //Inititalizations
    init_memory();
    init_free_list();
    initialize_queue(&READY_QUEUE);
    initialize_priority_queue(&READY_PRIORITY_QUEUE);
     selected_Algorithm_NUM=atoi(argv[1]);
    TIME_QUANTUM=atoi(argv[2]);// if RR 3
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    //key_t key_sch_process = ftok("keyfile_sch", 'B');
    //MESSAGE_sch_ID = msgget(key_sch_process, 0666|IPC_CREAT);
    //printf("queue id  is: %d\n",MESSAGE_ID);
    //
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    
    message_buf PROCESS_MESSAGE;
    process_count=0;

    if (selected_Algorithm_NUM == 1) {
        signal(SIGUSR1, myHandler);
    } 

    /*---------------------------Omar Syed------------------------------------*/

    while(1)
    {

        for(int i=0;i<process_count;i++){
            if (pcb[i].blocked_time<=0) {
                process_Node* unblocked_process = dequeue(&BLOCKED_QUEUE);
                if (unblocked_process != NULL) {
                    printf("Process %d unblocking at time %d\n", pcb[i].process_id, getClk());
                    enqueue(&READY_QUEUE, unblocked_process->Process);
                    pcb[i].process_state = Ready;
                    // Log unblocking
                    pFile = fopen("scheduler.log", "a");
                    if(pFile) {
                        fprintf(pFile, "At time %-5d process %-5d unblocked arr %-5d total %-5d remain %-5d wait %-5d\n",
                                getClk(), pcb[i].process_id,
                                pcb[i].arrival_time,
                                pcb[i].RUNNING_TIME,
                                pcb[i].REMAINING_TIME,
                                pcb[i].WAITING_TIME);
                        fclose(pFile);
                }
            }
          if(pcb[i].process_state==Blocked){
            pcb[i].blocked_time--;
          }

        }
    }

        int rec_status = msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(process),2,IPC_NOWAIT);
        total_running_time[running_count]=PROCESS_MESSAGE.p.RUNNING_TIME;
        if(rec_status!=-1)
        {
            switch(selected_Algorithm_NUM) {
                case 1:
                    // HPF
                    PCB* p = malloc(sizeof(PCB));
                    p->process_id = PROCESS_MESSAGE.p.ID;
                    p->priority = PROCESS_MESSAGE.p.PRIORITY;
                    p->dependency_id = PROCESS_MESSAGE.p.DEPENDENCY_ID;
                    p->REMAINING_TIME = PROCESS_MESSAGE.p.RUNNING_TIME;
                    p->RUNNING_TIME = PROCESS_MESSAGE.p.RUNNING_TIME;
                    p->arrival_time = PROCESS_MESSAGE.p.ARRIVAL_TIME;
                    p->STARTED = false;
                    p->is_completed = false;
                    p->process_state = Ready;

                    pcbArray[pcbCount++] = p;
                    enqueuePriority(&readyPriorityQueue, p);
                    break;
                case 2:

        enqueue_priority_SRTN(&READY_PRIORITY_QUEUE, PROCESS_MESSAGE.p);

        //    PRINT_READY_PRIORITY_QUEUE();

        running_process_index = get_pcb_index(pcb, process_count, peek_priority_front(&READY_PRIORITY_QUEUE)->ID);

        if (running_process_index == -1 && peek_priority_front(&READY_PRIORITY_QUEUE) != NULL && peek_priority_front(&READY_PRIORITY_QUEUE)->first_time) {
            current_time = getClk();
            peek_priority_front(&READY_PRIORITY_QUEUE)->first_time = false;

            pcb[process_count].arrival_time = peek_priority_front(&READY_PRIORITY_QUEUE)->ARRIVAL_TIME;
            pcb[process_count].process_id = peek_priority_front(&READY_PRIORITY_QUEUE)->ID;
            pcb[process_count].RUNNING_TIME = peek_priority_front(&READY_PRIORITY_QUEUE)->RUNNING_TIME;
            pcb[process_count].REMAINING_TIME = pcb[process_count].RUNNING_TIME;
            pcb[process_count].START_TIME = getClk();
            pcb[process_count].LAST_EXECUTED_TIME = getClk();
            pcb[process_count].process_state = Running;
            pcb[process_count].WAITING_TIME = 0;

            char str_rem_time[20];
            sprintf(str_rem_time, "%d", pcb[process_count].REMAINING_TIME);

            int pid = fork();
            if (pid == 0) {
                execl("./process.out", "./process.out", str_rem_time, NULL);
                exit(1);
            }

            pcb[process_count].process_pid = pid;
            running_process_index = process_count;
            process_count++;

            pFile = fopen("scheduler.log", "a");
            if (pFile) {
                fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                        current_time, pcb[running_process_index].process_id,
                        pcb[running_process_index].arrival_time,
                        pcb[running_process_index].RUNNING_TIME,
                        pcb[running_process_index].REMAINING_TIME,
                        current_time - pcb[running_process_index].arrival_time);
                fclose(pFile);
            }
    }

    if (running_process_index != -1 && peek_priority_front(&READY_PRIORITY_QUEUE) != NULL &&
        peek_priority_front(&READY_PRIORITY_QUEUE)->first_time) {

        if (pcb[running_process_index].REMAINING_TIME > peek_priority_front(&READY_PRIORITY_QUEUE)->RUNNING_TIME) {
            kill(pcb[running_process_index].process_pid, SIGSTOP);
            pcb[running_process_index].process_state = Ready;
        }

        current_time = getClk();
        peek_priority_front(&READY_PRIORITY_QUEUE)->first_time = false;

        pcb[process_count].arrival_time = peek_priority_front(&READY_PRIORITY_QUEUE)->ARRIVAL_TIME;
        pcb[process_count].process_id = peek_priority_front(&READY_PRIORITY_QUEUE)->ID;
        pcb[process_count].RUNNING_TIME = peek_priority_front(&READY_PRIORITY_QUEUE)->RUNNING_TIME;
        pcb[process_count].REMAINING_TIME = pcb[process_count].RUNNING_TIME;
        pcb[process_count].START_TIME = getClk();
        pcb[process_count].LAST_EXECUTED_TIME = getClk();
        pcb[process_count].process_state = Running;
        pcb[process_count].WAITING_TIME = 0;

        char str_rem_time2[20];
        sprintf(str_rem_time2, "%d", pcb[process_count].REMAINING_TIME);

        int pid = fork();
        if (pid == 0) {
            execl("./process.out", "./process.out", str_rem_time2, NULL);
            exit(1);
        }

        pcb[process_count].process_pid = pid;
        running_process_index = process_count;
        process_count++;

        pFile = fopen("scheduler.log", "a");
        if (pFile) {
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
                case 3:{
                              enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);

                    running_process_index = get_pcb_index(pcb, process_count, peek_front(&READY_QUEUE)->Process.ID);

                            if(running_process_index == -1 && peek_front(&READY_QUEUE) != NULL && peek_front(&READY_QUEUE)->Process.first_time) {

                                pcb[process_count].arrival_time = peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                                pcb[process_count].process_id = peek_front(&READY_QUEUE)->Process.ID;
                                pcb[process_count].RUNNING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                                pcb[process_count].REMAINING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;

                                pcb[process_count].process_pid = -1; 

                                
                                pcb[process_count].START_TIME = -1; 
                                pcb[process_count].process_state = Ready; 
                                pcb[process_count].WAITING_TIME = 0;

                                pcb[process_count].disk_base = peek_front(&READY_QUEUE)->Process.disk_base; 
                                pcb[process_count].limit = peek_front(&READY_QUEUE)->Process.limit;
                                pcb[process_count].num_pages = (pcb[process_count].limit + PAGE_SIZE - 1) / PAGE_SIZE;

                                init_process_page_table(&pcb[process_count]);
                            
                                //  REQUEST PAGE 0
                                int fault = Request(pcb, process_count, pcb[process_count].process_id, 0, 'r');
                            
                                if (fault) {
                                    pcb[process_count].process_state = Blocked;
                                    pcb[process_count].blocked_time = DISK_ACCESS_TIME;

                                    process_count++; 
                                    printf("Process %d blocked due to page fault on initial request.\n", pcb[process_count - 1].process_id);
                                    process_Node* Blocked = dequeue(&READY_QUEUE);
                                    enqueue(&BLOCKED_QUEUE, Blocked->Process);
                                    continue; // Skip execution
                                }
                                current_time = getClk();
                                peek_front(&READY_QUEUE)->Process.first_time = false;
                                pcb[process_count].START_TIME = current_time;
                                pcb[process_count].LAST_EXECUTED_TIME = current_time;
                                pcb[process_count].process_state = Running;
                            
                                char str_rem_time[20];
                                sprintf(str_rem_time, "%d", peek_front(&READY_QUEUE)->Process.RUNNING_TIME);

                                int pid = fork();

                                if(pid == 0) {
                                    execl("./process.out", "./process.out", str_rem_time, NULL);
                                    perror("Error in execl");
                                    exit(1);
                                }
                            
                                pcb[process_count].process_pid = pid;
                                running_process_index = process_count;
                                process_count++;
                            
                                // Logging
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

        if ((selected_Algorithm_NUM == 1 && timer != getClk()) || childFinished) {
            timer = getClk();

            hpfLoop(timer);

            childFinished = 0;
        }

        //SRTN
        if (selected_Algorithm_NUM == 2 && clock_timer != getClk()) {
    clock_timer = getClk();
    printf("Clock Timer : %d \n",getClk());
    //PRINT_READY_PRIORITY_QUEUE();

    if (running_process_index != -1 && pcb[running_process_index].process_state == Running)
        pcb[running_process_index].REMAINING_TIME--;

    if (peek_priority_front(&READY_PRIORITY_QUEUE) == NULL)
        continue;

    if (running_process_index != -1 && pcb[running_process_index].process_state == Running)
        total_running_times++;

    process_PNode* temp = READY_PRIORITY_QUEUE.front;
    while (temp != NULL) {
        int index = get_pcb_index(pcb, process_count, temp->Process.ID);
        if (index != -1 && pcb[index].process_state == Ready)
            pcb[index].WAITING_TIME++;
        temp = temp->next;
    }

    int front_index = get_pcb_index(pcb, process_count, peek_priority_front(&READY_PRIORITY_QUEUE)->ID);

    if (running_process_index != -1 && front_index != -1 && pcb[front_index].REMAINING_TIME < pcb[running_process_index].REMAINING_TIME) {
        kill(pcb[running_process_index].process_pid, SIGSTOP);
        pcb[running_process_index].process_state = Ready;
        pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
    }

    running_process_index = get_pcb_index(pcb, process_count, peek_priority_front(&READY_PRIORITY_QUEUE)->ID);

    if (running_process_index == -1 && peek_priority_front(&READY_PRIORITY_QUEUE)->first_time) {
        current_time = getClk();
        peek_priority_front(&READY_PRIORITY_QUEUE)->first_time = false;

        pcb[process_count].arrival_time = peek_priority_front(&READY_PRIORITY_QUEUE)->ARRIVAL_TIME;
        pcb[process_count].process_id = peek_priority_front(&READY_PRIORITY_QUEUE)->ID;
        pcb[process_count].RUNNING_TIME = peek_priority_front(&READY_PRIORITY_QUEUE)->RUNNING_TIME;
        pcb[process_count].REMAINING_TIME = pcb[process_count].RUNNING_TIME;
        pcb[process_count].START_TIME = getClk();
        pcb[process_count].LAST_EXECUTED_TIME = getClk();
        pcb[process_count].process_state = Running;
        pcb[process_count].WAITING_TIME = 0;

        char str_rem_time[20];
        sprintf(str_rem_time, "%d", pcb[process_count].REMAINING_TIME);

        int pid = fork();
        if (pid == 0) {
            execl("./process.out", "./process.out", str_rem_time, NULL);
            exit(1);
        }

        pcb[process_count].process_pid = pid;
        running_process_index = process_count;
        process_count++;

        pFile = fopen("scheduler.log", "a");
        if (pFile) {
            fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                    current_time, pcb[running_process_index].process_id,
                    pcb[running_process_index].arrival_time,
                    pcb[running_process_index].RUNNING_TIME,
                    pcb[running_process_index].REMAINING_TIME,
                    current_time - pcb[running_process_index].arrival_time);
            fclose(pFile);
        }
    }

    if (running_process_index != -1) {
        if (pcb[running_process_index].REMAINING_TIME <= 0) {

            pcb[running_process_index].process_state = Finished;
            pcb[running_process_index].FINISH_TIME = getClk();
            pcb[running_process_index].is_completed = true;
            pcb[running_process_index].WAITING_TIME =
                pcb[running_process_index].FINISH_TIME -
                pcb[running_process_index].arrival_time -
                pcb[running_process_index].RUNNING_TIME;

            pFile = fopen("scheduler.log", "a");
            if (pFile) {
                fprintf(pFile,
                        "At time %-5d process %-5d finished arr %-5d total %-5d remain %-5d wait %-5d TA %-5d WTA %.2f\n",
                        getClk(), pcb[running_process_index].process_id,
                        pcb[running_process_index].arrival_time,
                        pcb[running_process_index].RUNNING_TIME,
                        pcb[running_process_index].REMAINING_TIME,
                        pcb[running_process_index].WAITING_TIME,
                        pcb[running_process_index].FINISH_TIME - pcb[running_process_index].arrival_time,
                        (float)(pcb[running_process_index].FINISH_TIME - pcb[running_process_index].arrival_time) /
                        pcb[running_process_index].RUNNING_TIME);
                fclose(pFile);
            }

           WTA[count] = (float)(pcb[running_process_index].FINISH_TIME - pcb[running_process_index].arrival_time) / pcb[running_process_index].RUNNING_TIME;
            wait_time[count] = pcb[running_process_index].WAITING_TIME;
            count++;

            dequeue_priority(&READY_PRIORITY_QUEUE);

            kill(pcb[running_process_index].process_pid, SIGUSR2);

            remove_pcb(pcb, &process_count, pcb[running_process_index].process_id);
            finished_process++;

            if (finished_process == total_process)
                continue;

            if (peek_priority_front(&READY_PRIORITY_QUEUE) != NULL) {
                running_process_index = get_pcb_index(pcb, process_count, peek_priority_front(&READY_PRIORITY_QUEUE)->ID);

                if (running_process_index != -1 &&pcb[running_process_index].process_state == Ready &&peek_priority_front(&READY_PRIORITY_QUEUE)->first_time!=true) {

                    kill(pcb[running_process_index].process_pid, SIGCONT);
                    pcb[running_process_index].process_state = Running;
                    pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
                    pcb[running_process_index].WAITING_TIME =getClk() - pcb[running_process_index].arrival_time -(pcb[running_process_index].RUNNING_TIME - pcb[running_process_index].REMAINING_TIME);

                    pFile = fopen("scheduler.log", "a");
                    if (pFile) {
                        fprintf(pFile,
                                "At time %-5d process %-5d resumed arr %-5d total %-5d remain %-5d wait %-5d\n",
                                getClk(), pcb[running_process_index].process_id,
                                pcb[running_process_index].arrival_time,
                                pcb[running_process_index].RUNNING_TIME,
                                pcb[running_process_index].REMAINING_TIME,
                                pcb[running_process_index].WAITING_TIME);
                        fclose(pFile);
                    }

                } else if (running_process_index != -1 &&pcb[running_process_index].process_state == Ready &&peek_priority_front(&READY_PRIORITY_QUEUE)->first_time) {

                    current_time = getClk();
                    peek_priority_front(&READY_PRIORITY_QUEUE)->first_time = false;

                    pcb[process_count].arrival_time = peek_priority_front(&READY_PRIORITY_QUEUE)->ARRIVAL_TIME;
                    pcb[process_count].process_id = peek_priority_front(&READY_PRIORITY_QUEUE)->ID;
                    pcb[process_count].RUNNING_TIME = peek_priority_front(&READY_PRIORITY_QUEUE)->RUNNING_TIME;
                    pcb[process_count].REMAINING_TIME = pcb[process_count].RUNNING_TIME;
                    pcb[process_count].START_TIME = getClk();
                    pcb[process_count].LAST_EXECUTED_TIME = getClk();
                    pcb[process_count].process_state = Running;
                    pcb[process_count].WAITING_TIME = 0;

                    char str_rem_time2[20];
                    sprintf(str_rem_time2, "%d", pcb[process_count].REMAINING_TIME);

                    int pid = fork();
                    if (pid == 0) {
                        execl("./process.out", "./process.out", str_rem_time2, NULL);
                        exit(1);
                    }

                    pcb[process_count].process_pid = pid;
                    running_process_index = process_count;
                    process_count++;

                    pFile = fopen("scheduler.log", "a");
                    if (pFile) {
                        fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                                current_time, pcb[running_process_index].process_id,
                                pcb[running_process_index].arrival_time,
                                pcb[running_process_index].RUNNING_TIME,
                                pcb[running_process_index].REMAINING_TIME,
                                current_time - pcb[running_process_index].arrival_time);
                        fclose(pFile);
                    }
                }
            }
        }
    }
}

      
        if(selected_Algorithm_NUM == 3 && clock_timer != getClk()) {
                clock_timer = getClk();
                printf("Clock Timer : %d \n", clock_timer);

            process_Node* temp_process = BLOCKED_QUEUE.front;
    while(temp_process != NULL) {
        int index = get_pcb_index(pcb, process_count, temp_process->Process.ID);
        
        if(index != -1 && pcb[index].process_state == Blocked) {
            pcb[index].blocked_time--;
            
            // Check if blocking period is over
            if(pcb[index].blocked_time <= 0) {
                printf("Process %d unblocking at time %d\n", pcb[index].process_id, getClk());
                
                // Remove from blocked queue and add to ready queue
                process_Node* unblocked_process = dequeue(&BLOCKED_QUEUE);
                enqueue(&READY_QUEUE, unblocked_process->Process);
                pcb[index].process_state = Ready;
                
                // If process was never started (blocked on first page load), fork it now
                if(pcb[index].process_pid == -1) {
                    printf("Forking process %d after unblocking\n", pcb[index].process_id);
                    
                    char str_rem_time[20];
                    sprintf(str_rem_time, "%d", pcb[index].REMAINING_TIME);
                    
                    int pid = fork();
                    if(pid == 0) {
                        execl("./process.out", "./process.out", str_rem_time, NULL);
                        perror("Error in execl");
                        exit(1);
                    }
                    
                    pcb[index].process_pid = pid;
                    
                    // Immediately stop it since it's going to ready queue, not running
                    kill(pcb[index].process_pid, SIGSTOP);
                    
                    // Set start time if not set
                    if(pcb[index].START_TIME == -1) {
                        pcb[index].START_TIME = getClk();
                    }
                }
                
                // Log unblocking
                pFile = fopen("scheduler.log", "a");
                if(pFile) {
                    fprintf(pFile, "At time %-5d process %-5d unblocked arr %-5d total %-5d remain %-5d wait %-5d\n",
                            getClk(), pcb[index].process_id,
                            pcb[index].arrival_time,
                            pcb[index].RUNNING_TIME,
                            pcb[index].REMAINING_TIME,
                            pcb[index].WAITING_TIME);
                    fclose(pFile);
                }
            }
        }
        
        temp_process = temp_process->next;
        if(temp_process == BLOCKED_QUEUE.front) {
            break;
        }
    }

    // Process running process if exists
    if(running_process_index != -1 && pcb[running_process_index].process_state == Running) {
        // Check if this process has memory requests
        if(pcb[running_process_index].num_requests > 0) {
            request current_request = pcb[running_process_index].memory_requests[0];
            
            // Check if request should be processed at current time
            if(current_request.time == pcb[running_process_index].execution_time) {
                printf("Process %d requesting memory at time %d (address=%d, rw=%c)\n", 
                       pcb[running_process_index].process_id, getClk(), 
                       current_request.address, current_request.rw);
                
                int virtual_page = get_vpn(current_request.address);
                int page_fault = Request(pcb, process_count, 
                                        pcb[running_process_index].process_id, 
                                        virtual_page, current_request.rw);
                
                // Remove this request from the list
                for(int i = 1; i < pcb[running_process_index].num_requests; i++) {
                    pcb[running_process_index].memory_requests[i - 1] = 
                        pcb[running_process_index].memory_requests[i];
                }
                pcb[running_process_index].num_requests--;
                
                // If page fault occurred, block the process
                if(page_fault == 1) {
                    printf("Page fault! Blocking process %d\n", pcb[running_process_index].process_id);
                    
                    // Move to blocked queue
                    process_Node* process_node = dequeue(&READY_QUEUE);
                    enqueue(&BLOCKED_QUEUE, process_node->Process);
                    
                    // Update PCB state
                    pcb[running_process_index].process_state = Blocked;
                    pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
                    pcb[running_process_index].blocked_time = DISK_ACCESS_TIME;
                    
                    // Stop the running process
                    kill(pcb[running_process_index].process_pid, SIGSTOP);
                    
                    // Log blocking
                    pFile = fopen("scheduler.log", "a");
                    if(pFile) {
                        fprintf(pFile, "At time %-5d process %-5d blocked arr %-5d total %-5d remain %-5d wait %-5d\n",
                                getClk(), pcb[running_process_index].process_id,
                                pcb[running_process_index].arrival_time,
                                pcb[running_process_index].RUNNING_TIME,
                                pcb[running_process_index].REMAINING_TIME,
                                pcb[running_process_index].WAITING_TIME);
                        fclose(pFile);
                    }
                    
                    // Clear running process to trigger scheduling
                    running_process_index = -1;
                }
            }
        }
        
        // Only decrement remaining time if process is still running (not blocked)
        if(running_process_index != -1 && pcb[running_process_index].process_state == Running) {
            pcb[running_process_index].REMAINING_TIME--;
            pcb[running_process_index].execution_time++;
            total_running_times++;
        }
    }           
                if(peek_front(&READY_QUEUE) == NULL) {
                    if(finished_process >= total_process) {
                        printf("All processes finished at time %d\n", getClk());
                        }
                    continue;  
                }

                // increment waiting time for ready processes
               // FIXED: Increment waiting time for processes in ready queue
// This should be placed AFTER blocked queue processing and BEFORE running process handling

if(peek_front(&READY_QUEUE) == NULL) {
    if(finished_process >= total_process) {
        // All processes finished
    }
    continue;
}

// Increment waiting time for ready processes
                process_Node* temp = READY_QUEUE.front;
                while(temp != NULL) {
                    int index = get_pcb_index(pcb, process_count, temp->Process.ID);
                    if(index != -1 && pcb[index].process_state == Ready) {
                        pcb[index].WAITING_TIME++;
                    }
                    temp = temp->next;
                    if(temp == READY_QUEUE.rear->next)
                        break;
                }
                
                // Also increment waiting time for blocked processes
                process_Node* temp_blocked = BLOCKED_QUEUE.front;
                while(temp_blocked != NULL) {
                    int index = get_pcb_index(pcb, process_count, temp_blocked->Process.ID);
                    if(index != -1 && pcb[index].process_state == Blocked) {
                        // Blocked processes are also waiting (just not in ready queue)
                        pcb[index].WAITING_TIME++;
                    }
                    temp_blocked = temp_blocked->next;
                    if(temp_blocked == BLOCKED_QUEUE.rear->next)
                        break;
                }


                // FIXED: Initial Process Loading - Handles page faults on first load
                running_process_index = get_pcb_index(pcb, process_count, peek_front(&READY_QUEUE)->Process.ID);

                if(running_process_index == -1 && peek_front(&READY_QUEUE)->Process.first_time) {

                    process* proc = &peek_front(&READY_QUEUE)->Process;

                    printf("Initializing new process %d at time %d\n", proc->ID, getClk());

                    // Initialize PCB fields
                    pcb[process_count].process_id = proc->ID;
                    pcb[process_count].arrival_time = proc->ARRIVAL_TIME;
                    pcb[process_count].RUNNING_TIME = proc->RUNNING_TIME;
                    pcb[process_count].REMAINING_TIME = proc->RUNNING_TIME;
                    pcb[process_count].WAITING_TIME = 0;
                    pcb[process_count].START_TIME = -1;  // Will be set when actually starts
                    pcb[process_count].LAST_EXECUTED_TIME = getClk();
                    pcb[process_count].execution_time = 0;
                    pcb[process_count].process_pid = -1;  // Not forked yet
                    pcb[process_count].blocked_time = 0;

                    // Initialize memory-related fields
                    pcb[process_count].disk_base = proc->disk_base;
                    pcb[process_count].limit = proc->limit;
                    pcb[process_count].num_pages = (proc->limit + PAGE_SIZE - 1) / PAGE_SIZE;
                    pcb[process_count].num_requests = proc->num_requests;

                    // Copy memory requests
                    for(int i = 0; i < proc->num_requests; i++) {
                        pcb[process_count].memory_requests[i] = proc->memory_requests[i];
                    }

                    printf("Process %d: needs %d pages, has %d memory requests\n", 
                           proc->ID, pcb[process_count].num_pages, pcb[process_count].num_requests);
                    
                    // Initialize page table
                    int pt_result = init_process_page_table(&pcb[process_count]);
                    if(pt_result == -1) {
                        printf("ERROR: Failed to initialize page table for process %d\n", proc->ID);
                        dequeue(&READY_QUEUE);
                        // Continue to next iteration without incrementing process_count
                        continue;
                    }

                    int pt_page = allocate_process_page_table(&pcb[process_count], pcb, process_count);
                    if(pt_page == -1) {
                        printf("ERROR: Failed to allocate page table for process %d\n", proc->ID);
                        dequeue(&READY_QUEUE);
                        continue;
                    }

                    printf("Process %d: Page table allocated at physical frame %d\n", proc->ID, pt_page);

                    // Try to load first page (page 0) - this may cause a page fault
                    int page_fault = Request(pcb, process_count, pcb[process_count].process_id, 0, 'R');

                    proc->first_time = false;

                    if(page_fault == 1) {
                        // PAGE FAULT on first load - block the process without forking
                        printf("Page fault on first load for process %d - blocking\n", proc->ID);

                        process_Node* process_node = dequeue(&READY_QUEUE);
                        enqueue(&BLOCKED_QUEUE, process_node->Process);

                        pcb[process_count].process_state = Blocked;
                        pcb[process_count].blocked_time = DISK_ACCESS_TIME;

                        // Log blocking (not started yet, so use "blocked on arrival")
                        pFile = fopen("scheduler.log", "a");
                        if(pFile) {
                            fprintf(pFile, "At time %-5d process %-5d blocked on first load arr %-5d total %-5d remain %-5d wait %-5d\n",
                                    getClk(), pcb[process_count].process_id,
                                    pcb[process_count].arrival_time,
                                    pcb[process_count].RUNNING_TIME,
                                    pcb[process_count].REMAINING_TIME,
                                    pcb[process_count].WAITING_TIME);
                            fclose(pFile);
                        }

                        running_process_index = -1;
                        process_count++;

                    } else {
                        // NO PAGE FAULT - start process immediately
                        printf("No page fault for process %d - starting immediately\n", proc->ID);

                        pcb[process_count].process_state = Running;
                        pcb[process_count].START_TIME = getClk();

                        char str_rem_time[20];
                        sprintf(str_rem_time, "%d", pcb[process_count].REMAINING_TIME);

                        int pid = fork();
                        if(pid == 0) {
                            execl("./process.out", "./process.out", str_rem_time, NULL);
                            perror("Error in execl");
                            exit(1);
                        }

                        pcb[process_count].process_pid = pid;
                        running_process_index = process_count;
                        process_count++;

                        // Log start
                        pFile = fopen("scheduler.log", "a");
                        if(pFile) {
                            fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                                    getClk(), pcb[running_process_index].process_id,
                                    pcb[running_process_index].arrival_time,
                                    pcb[running_process_index].RUNNING_TIME,
                                    pcb[running_process_index].REMAINING_TIME,
                                    pcb[running_process_index].WAITING_TIME);
                            fclose(pFile);
                        }
                    }
                }
                
                if(running_process_index == -1) // if there are no ready processes
                continue;
    
   
              // FIXED: Process Termination - Add memory cleanup
            if(running_process_index != -1 && 
               pcb[running_process_index].process_state == Running &&
               pcb[running_process_index].REMAINING_TIME <= 0) {
                
                printf("Process %d finishing at time %d\n", pcb[running_process_index].process_id, getClk());
                
                // Update PCB state
                pcb[running_process_index].process_state = Finished;
                pcb[running_process_index].FINISH_TIME = getClk();
                pcb[running_process_index].is_completed = true;
                pcb[running_process_index].REMAINING_TIME = 0;
                
                // Calculate metrics
                int turnaround_time = pcb[running_process_index].FINISH_TIME - 
                                      pcb[running_process_index].arrival_time;
                float wta = (float)turnaround_time / pcb[running_process_index].RUNNING_TIME;
                
                // Log finished
                pFile = fopen("scheduler.log", "a");
                if(pFile) {
                    fprintf(pFile, "At time %-5d process %-5d finished arr %-5d total %-5d remain %-5d wait %-5d TA %-5d WTA %.2f\n",
                            getClk(), pcb[running_process_index].process_id,
                            pcb[running_process_index].arrival_time,
                            pcb[running_process_index].RUNNING_TIME,
                            pcb[running_process_index].REMAINING_TIME,
                            pcb[running_process_index].WAITING_TIME,
                            turnaround_time,
                            wta);
                    fclose(pFile);
                }
                
                // Store metrics
                WTA[count] = wta;
                wait_time[count] = pcb[running_process_index].WAITING_TIME;
                count++;
                
                // **CRITICAL FIX: Free memory resources**
                printf("Freeing memory for process %d\n", pcb[running_process_index].process_id);
                free_process_pages(pcb[running_process_index].process_id, &pcb[running_process_index]);
                
                // Remove from ready queue
                process_Node* removed = dequeue(&READY_QUEUE);
                
                // Terminate the process
                kill(pcb[running_process_index].process_pid, SIGUSR2);
                
                // Remove from PCB array
                int finished_id = pcb[running_process_index].process_id;
                remove_pcb(pcb, &process_count, finished_id);
                finished_process++;
                
                printf("Process %d cleaned up. Finished processes: %d/%d\n", 
                       finished_id, finished_process, total_process);
                
                // Check if all processes finished
                if(finished_process == total_process) {
                    continue;
                }
                
                // Schedule next process
                if(peek_front(&READY_QUEUE) != NULL) {
                    running_process_index = get_pcb_index(pcb, process_count, 
                                                           peek_front(&READY_QUEUE)->Process.ID);
                    
                    // If process exists and is ready, resume it
                    if(running_process_index != -1 && 
                       pcb[running_process_index].process_state == Ready) {
                        
                        kill(pcb[running_process_index].process_pid, SIGCONT);
                        pcb[running_process_index].process_state = Running;
                        pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
                        
                        pFile = fopen("scheduler.log", "a");
                        if(pFile) {
                            fprintf(pFile, "At time %-5d process %-5d resumed arr %-5d total %-5d remain %-5d wait %-5d\n",
                                    getClk(), pcb[running_process_index].process_id,
                                    pcb[running_process_index].arrival_time,
                                    pcb[running_process_index].RUNNING_TIME,
                                    pcb[running_process_index].REMAINING_TIME,
                                    pcb[running_process_index].WAITING_TIME);
                            fclose(pFile);
                        }
                    }
                    // Handle new process arrival (first_time = true)
                    else if(running_process_index == -1 && 
                            peek_front(&READY_QUEUE)->Process.first_time) {
                        // This will be handled in the next iteration by the initial load logic
                    }
                }
            }


    if(running_process_index != -1 && 
       pcb[running_process_index].process_state == Running &&
       pcb[running_process_index].REMAINING_TIME > 0) {
        
        int time_executed = getClk() - pcb[running_process_index].LAST_EXECUTED_TIME;
        
        if(time_executed >= TIME_QUANTUM) 
        {
          
            kill(pcb[running_process_index].process_pid, SIGSTOP);
            pcb[running_process_index].process_state = Ready;
            pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
          
            pFile = fopen("scheduler.log", "a");
            if(pFile) {
                fprintf(pFile, "At time %-5d process %-5d stopped arr %-5d total %-5d remain %-5d wait %-5d\n",
                        getClk(), pcb[running_process_index].process_id,
                        pcb[running_process_index].arrival_time,
                        pcb[running_process_index].RUNNING_TIME,
                        pcb[running_process_index].REMAINING_TIME,
                        pcb[running_process_index].WAITING_TIME);
                fclose(pFile);
            }
            
          
            process_Node* temp = dequeue(&READY_QUEUE);
            enqueue(&READY_QUEUE, temp->Process);
            
            running_process_index = get_pcb_index(pcb, process_count, 
                                                   peek_front(&READY_QUEUE)->Process.ID);
            
          
            if(running_process_index == -1 && 
               peek_front(&READY_QUEUE)->Process.first_time) {

                                pcb[running_process_index].arrival_time = peek_front(&READY_QUEUE)->Process.ARRIVAL_TIME;
                                pcb[running_process_index].process_id = peek_front(&READY_QUEUE)->Process.ID;
                                pcb[running_process_index].RUNNING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;
                                pcb[running_process_index].REMAINING_TIME = peek_front(&READY_QUEUE)->Process.RUNNING_TIME;

                                pcb[running_process_index].process_pid = -1; 

                                
                                pcb[running_process_index].START_TIME = -1; 
                                pcb[running_process_index].process_state = Ready; // Kept as Ready until we actually run or block
                                pcb[running_process_index].WAITING_TIME = 0;

                                pcb[running_process_index].disk_base = peek_front(&READY_QUEUE)->Process.disk_base; // Don't forget disk_base!
                                pcb[running_process_index].limit = peek_front(&READY_QUEUE)->Process.limit;
                                pcb[running_process_index].num_pages = (pcb[running_process_index].limit + PAGE_SIZE - 1) / PAGE_SIZE;

                                init_process_page_table(&pcb[running_process_index]);
                            
                                //  REQUEST PAGE 0
                                int fault = Request(pcb, process_count, pcb[running_process_index].process_id, 0, 'r');
                            
                                if (fault) {
                                    pcb[running_process_index].process_state = Blocked;
                                    pcb[running_process_index].blocked_time = DISK_ACCESS_TIME;
                                    process_count++; 
                                     printf("Process %d blocked due to page fault on initial request.\n", pcb[process_count - 1].process_id);
                                    process_Node* Blocked = dequeue(&READY_QUEUE);
                                    enqueue(&BLOCKED_QUEUE, Blocked->Process);
                                    continue; // Skip execution
                                }

                                current_time = getClk();
                                peek_front(&READY_QUEUE)->Process.first_time = false;
                                pcb[running_process_index].START_TIME = current_time;
                                pcb[running_process_index].LAST_EXECUTED_TIME = current_time;
                                pcb[running_process_index].process_state = Running;
                            
                                char str_rem_time[20];
                                sprintf(str_rem_time, "%d", peek_front(&READY_QUEUE)->Process.RUNNING_TIME);
                                printf("forked process %d at time %d\n", peek_front(&READY_QUEUE)->Process.ID, getClk());

                                int pid = fork();

                                if(pid == 0) {
                                    execl("./process.out", "./process.out", str_rem_time, NULL);
                                    perror("Error in execl");
                                    exit(1);
                                }
                            
                                pcb[running_process_index].process_pid = pid;

                                // Logging
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
         
            else if(running_process_index != -1 && pcb[running_process_index].REMAINING_TIME > 0) {
                          if  (pcb[running_process_index].num_requests > 0) 
                          {
                                 request current_request = pcb[running_process_index].memory_requests[0];
                                 if (current_request.time == getClk()) 
                                 {
                                     int Virtual_page = get_vpn(current_request.address);
                                     int fault = Request(pcb, process_count, pcb[running_process_index].process_id, Virtual_page, current_request.rw);
                                     for (int i = 1; i < pcb[running_process_index].num_requests; i++) {
                                         pcb[running_process_index].memory_requests[i - 1] = pcb[running_process_index].memory_requests[i];
                                     }
                                     pcb[running_process_index].num_requests--;
                                     if (fault == 1) {
                                         process_Node* process_node = dequeue(&READY_QUEUE);
                                         enqueue(&BLOCKED_QUEUE, process_node->Process);
                                         printf("Process %d blocked due to page fault at time %d\n",pcb[running_process_index].process_id,getClk());
                                         pFile = fopen("scheduler.log", "a");
                                            if(pFile) {
                                                fprintf(pFile, "At time %-5d process %-5d blocked arr %-5d total %-5d remain %-5d wait %-5d\n",
                                                        getClk(), pcb[running_process_index].process_id,
                                                        pcb[running_process_index].arrival_time,
                                                        pcb[running_process_index].RUNNING_TIME,
                                                        pcb[running_process_index].REMAINING_TIME,
                                                        pcb[running_process_index].WAITING_TIME);
                                                fclose(pFile);
                                            }
                                         pcb[running_process_index].process_state = Blocked;
                                         pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
                                         pcb[running_process_index].blocked_time = DISK_ACCESS_TIME; // BLOCK TIME
                                            kill(pcb[running_process_index].process_pid, SIGSTOP);
                                     }
                                 }

                } 
                else
                {
                    kill(pcb[running_process_index].process_pid, SIGCONT);
                    pcb[running_process_index].process_state = Running;
                    pcb[running_process_index].LAST_EXECUTED_TIME = getClk();
                    pFile = fopen("scheduler.log", "a");
                    if(pFile) 
                    {
                       fprintf(pFile, "At time %-5d process %-5d resumed arr %-5d total %-5d remain %-5d wait %-5d\n",
                               getClk(), pcb[running_process_index].process_id,
                               pcb[running_process_index].arrival_time,
                               pcb[running_process_index].RUNNING_TIME,
                               pcb[running_process_index].REMAINING_TIME,
                               pcb[running_process_index].WAITING_TIME);
                       fclose(pFile);
                    }
                }
                
                
            }
        }
    }
}

        if(finished_process==total_process){
            float AVGWAITING=0;
            float AVGWTA = 0;
            float running = 0;
            for(int i=0;i<count;i++){
                AVGWTA+=WTA[i];
                AVGWAITING+=wait_time[i];
                running+=total_running_time[i];
            }
            
            AVGWAITING=AVGWAITING/total_process;
            AVGWTA=AVGWTA/total_process;
            
            for(int i=0;i<count;i++){
                std_dev_sqr+=pow((WTA[i]-AVGWTA),2);
            }
            std_dev_sqr=std_dev_sqr/total_process;
            float std_dev = sqrt(std_dev_sqr);
            
            pFile=fopen("scheduler.perf", "w");
            if(pFile) {
                fprintf(pFile, "CPU utilization = %.2f%%\nAvg WTA = %.2f\nAvg Waiting = %.2f\nStd WTA = %.2f\n ", 
                    ((float)(total_running_times)/getClk())*100, 
                    AVGWTA, 
                    AVGWAITING,std_dev );
                    fclose(pFile);
                    printf("\nPerformance File Has Been Generated !\a\n");
                }
                finished_process=0;
                total_process=-1;
                
            }



        }
        
    return 0;
    destroyClk(true);
}