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

/*---------------------------------Global Variables------------------------------------*/
int MESSAGE_ID;
int finished_process = 0;
PCB pcb[max];
FILE* pFile;
int* wait_time;
int* total_running_time;
int total_running_times = 0;
float* WTA;
int running_count = 0;
float std_dev_sqr = 0;
int count = 0;
int selected_Algorithm_NUM = -1;

process_queue READY_QUEUE;
process_priority_queue READY_PRIORITY_QUEUE;
process_queue BLOCKED_QUEUE;
int TIME_QUANTUM;
int current_clock = 0;
int running_process_index = -1;
int process_count = 0;
int total_process = 0;

int quantum_counter = 0;



DiskOperation disk_queue[100];
int disk_queue_size = 0;

PCB* pcbArray[max];
int pcbCount = 0;
PCB* runningPcb = NULL;
PcbPriorityQueue readyPriorityQueue;
int timer = 0;
int childFinished = 0;
int finished_PCB = 0;

PCB* getPcbById(int processId) {
    for (int i = 0; i < pcbCount; i++)
        if (pcbArray[i]->process_id == processId)
            return pcbArray[i];
    return NULL;
}

PCB* getPcbByPid(int pid) {
    for (int i = 0; i < pcbCount; i++){
        if (pcbArray[i]->process_pid == pid){ 
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
        if (pid == 0) { 
            char timeArg[16];
            sprintf(timeArg, "%d", p->REMAINING_TIME);
            execl("./process2.out", "./process2.out", timeArg, NULL);
            exit(1);
        }

        p->STARTED = true;
        p->process_pid = pid;
        p->process_state = Running;
        p->START_TIME = currentTick;
        p->LAST_EXECUTED_TIME = currentTick;

        printLog(p, "started");
    } else {
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

    childFinished = 1;
    printPCB(finished);
}

PCB* findPCBByID(int id) {
    for (int i = 0; i < pcbCount; i++) {
        if (pcbArray[i]->process_id == id) return pcbArray[i];
    }
    return NULL;
}
//////////////////////////*/////////////////////////////


void PRINT_READY_QUEUE() {
    if (READY_QUEUE.front == NULL) {
        printf("Ready Queue: EMPTY\n");
        return;
    }
    
    process_Node* temp = READY_QUEUE.front;
    printf("Ready Queue: ");
    while (temp != NULL) {
        printf("P%d ", temp->Process.ID);
        temp = temp->next;
    }
    printf("\n");
}

void PRINT_BLOCKED_QUEUE() {
    if (BLOCKED_QUEUE.front == NULL) {
        printf("Blocked Queue: EMPTY\n");
        return;
    }
    
    process_Node* temp = BLOCKED_QUEUE.front;
    printf("Blocked Queue: ");
    while (temp != NULL) {
        printf("P%d ", temp->Process.ID);
        temp = temp->next;
    }
    printf("\n");
}

void handle_disk_completions(int current_time) {
    for (int i = 0; i < disk_queue_size; i++) {
        if (disk_queue[i].completion_time <= current_time && disk_queue[i].completion_time > 0) {
            int pid = disk_queue[i].process_id;
            
            process_Node* current_node = BLOCKED_QUEUE.front;
            process_Node* prev = NULL;
            
            while (current_node != NULL) {
                if (current_node->Process.ID == pid) {
                    if (prev == NULL) {
                        BLOCKED_QUEUE.front = current_node->next;
                    } else {
                        prev->next = current_node->next;
                    }
                    
                    enqueue(&READY_QUEUE, current_node->Process);
                    
                    for (int j = 0; j < process_count; j++) {
                        if (pcb[j].process_id == pid) {
                            pcb[j].process_state = Ready;
                            pcb[j].blocked_time = 0;
                            break;
                        }
                    }
                    
                    free(current_node);
                    break;
                }
                prev = current_node;
                current_node = current_node->next;
            }
            
            for (int j = i; j < disk_queue_size - 1; j++) {
                disk_queue[j] = disk_queue[j + 1];
            }
            disk_queue_size--;
            i--;
        }
    }
}

void add_disk_operation(int process_id, int virtual_page, int io_time, int current_time) {
    if (disk_queue_size >= 100) {
        printf("Error: Disk queue full!\n");
        return;
    }
    
    disk_queue[disk_queue_size].process_id = process_id;
    disk_queue[disk_queue_size].virtual_page = virtual_page;
    disk_queue[disk_queue_size].io_time_remaining = io_time;
    disk_queue[disk_queue_size].io_time_elapsed = 0;
    disk_queue[disk_queue_size].completion_time = current_time + io_time;
    disk_queue_size++;
}

int handle_memory_request(int process_index, int current_time) {
    PCB* p = &pcb[process_index];
    
    for (int i = 0; i < p->num_requests; i++) {
        if (p->memory_requests[i].time == p->execution_time) {
            int virtual_address = p->memory_requests[i].address;
            char rw = p->memory_requests[i].rw;
            int virtual_page = get_vpn(virtual_address);
            
           
            if (p->execution_time == 1) {
                printf("Process %d: First memory request at time %d - not blocking\n", 
                       p->process_id, p->execution_time);
                int io_time = Request(pcb, process_count, p->process_id, virtual_page, rw, current_time);
                return 0;
            }
            
            
            int io_time = Request(pcb, process_count, p->process_id, virtual_page, rw, current_time);
            
            if (io_time > 0) {
                return io_time;
            }
            break;
        }
    }
    return 0;
}

void start_process(int process_index, int current_time) {
    PCB* p = &pcb[process_index];
    
    if (!p->STARTED) {

        if (p->page_table.entries == NULL) {
            if (init_process_page_table(p) != 0) {
                printf("Error: Failed to initialize page table for process %d\n", p->process_id);
                return;
            }
            
            allocate_process_page_table(p);
            
            
            printf("Process %d: Loading first page without blocking (project assumption)\n", p->process_id);
            

            for (int i = 0; i < p->num_requests; i++) {
                if (p->memory_requests[i].time == 0) {
                    int virtual_address = p->memory_requests[i].address;
                    int virtual_page = get_vpn(virtual_address);
                    char rw = p->memory_requests[i].rw;
                    
                    int io_time = Request(pcb, process_count, p->process_id, virtual_page, rw, current_time);
                    if (io_time > 0) {

                        printf("Process %d: First page fault occurred but not blocking (first request)\n", p->process_id);
                    }
                    break;
                }
            }
        }
        
        char rem_time_str[20];
        sprintf(rem_time_str, "%d", p->REMAINING_TIME);
        
        int pid = fork();
        if (pid == 0) {
            execl("./process.out", "./process.out", rem_time_str, NULL);
            exit(1);
        }
        
        p->process_pid = pid;
        p->STARTED = 1;
        p->START_TIME = current_time;
        p->execution_time = 0;
        
  
        p->WAITING_TIME = current_time - p->arrival_time;
        if (p->WAITING_TIME < 0) p->WAITING_TIME = 0;
        
        pFile = fopen("scheduler.log", "a");
        if (pFile) {
            fprintf(pFile, "At time %-5d process %-5d started   arr %-5d total %-5d remain %-5d wait %-5d\n",
                    current_time, p->process_id,
                    p->arrival_time,
                    p->RUNNING_TIME,
                    p->REMAINING_TIME,
                    p->WAITING_TIME);
            fclose(pFile);
        }
    } else {
        if (p->process_pid > 0) {
            kill(p->process_pid, SIGCONT);
        }
        
        int time_spent_running = p->RUNNING_TIME - p->REMAINING_TIME;
        p->WAITING_TIME = current_time - p->arrival_time - time_spent_running;
        if (p->WAITING_TIME < 0) p->WAITING_TIME = 0;
        
        pFile = fopen("scheduler.log", "a");
        if (pFile) {
            fprintf(pFile, "At time %-5d process %-5d resumed   arr %-5d total %-5d remain %-5d wait %-5d\n",
                    current_time, p->process_id,
                    p->arrival_time,
                    p->RUNNING_TIME,
                    p->REMAINING_TIME,
                    p->WAITING_TIME);
            fclose(pFile);
        }
    }
    
    p->process_state = Running;
    p->LAST_EXECUTED_TIME = current_time;
    quantum_counter = 0;
}

void stop_process(int process_index, int current_time) {

    PCB* p = &pcb[process_index];
    
    if (p->process_pid > 0) {
        kill(p->process_pid, SIGSTOP);
    }
    
    p->process_state = Ready;
    
    int time_spent_running = p->RUNNING_TIME - p->REMAINING_TIME;
    p->WAITING_TIME = current_time - p->arrival_time - time_spent_running;
    if (p->WAITING_TIME < 0) 
        p->WAITING_TIME = 0;
    
    pFile = fopen("scheduler.log", "a");
    if (pFile) {
        fprintf(pFile, "At time %-5d process %-5d stopped   arr %-5d total %-5d remain %-5d wait %-5d\n",
                current_time, p->process_id,
                p->arrival_time,
                p->RUNNING_TIME,
                p->REMAINING_TIME,
                p->WAITING_TIME);
        fclose(pFile);
    }
}

void block_process(int process_index, int current_time, int io_time) {
    PCB* p = &pcb[process_index];
    
    if (p->process_pid > 0) {
        kill(p->process_pid, SIGSTOP);
    }
    
    p->process_state = Blocked;
    p->blocked_time = io_time;
    
    int time_spent_running = p->RUNNING_TIME - p->REMAINING_TIME;
    p->WAITING_TIME = current_time - p->arrival_time - time_spent_running;
    if (p->WAITING_TIME < 0) p->WAITING_TIME = 0;
    
    process temp_process;
    temp_process.ID = p->process_id;
    temp_process.ARRIVAL_TIME = p->arrival_time;
    temp_process.RUNNING_TIME = p->RUNNING_TIME;
    temp_process.PRIORITY = p->priority;
    temp_process.first_time = 0;
    temp_process.disk_base = p->disk_base;
    temp_process.limit = p->limit;
    temp_process.num_requests = p->num_requests;
    
    enqueue(&BLOCKED_QUEUE, temp_process);
    
    
    pFile = fopen("scheduler.log", "a");
    if (pFile) {
        fprintf(pFile, "At time %-5d process %-5d blocked   arr %-5d total %-5d remain %-5d wait %-5d\n",
                current_time, p->process_id,
                p->arrival_time,
                p->RUNNING_TIME,
                p->REMAINING_TIME,
                p->WAITING_TIME);
        fclose(pFile);
    }
    
    int virtual_page_for_fault = -1;
    for (int i = 0; i < p->num_requests; i++) {
        if (p->memory_requests[i].time == p->execution_time) {
            virtual_page_for_fault = get_vpn(p->memory_requests[i].address);
            break;
        }
    }
    
    if (virtual_page_for_fault >= 0) {
        add_disk_operation(p->process_id, virtual_page_for_fault, io_time, current_time);
    }
}

void finish_process(int process_index, int current_time) {
    PCB* p = &pcb[process_index];
    
    p->process_state = Finished;
    p->FINISH_TIME = current_time;
    p->is_completed = 1;
    
  
    int actual_cpu_time = p->RUNNING_TIME - p->REMAINING_TIME;
    p->WAITING_TIME = current_time - p->arrival_time - actual_cpu_time;
    if (p->WAITING_TIME < 0) p->WAITING_TIME = 0;
    
    free_process_pages(p->process_id, p);
    
    if (p->process_pid > 0) {
        kill(p->process_pid, SIGUSR2);
    }
    
    int TA = p->FINISH_TIME - p->arrival_time;
    float WTA_val = 0.0;
    if (p->RUNNING_TIME > 0) {
        WTA_val = (float)TA / p->RUNNING_TIME;
    }
    
    
    pFile = fopen("scheduler.log", "a");
    if (pFile) {
        fprintf(pFile, "At time %-5d process %-5d finished arr %-5d total %-5d remain %-5d wait %-5d TA %-5d WTA %.2f\n",
                current_time, p->process_id,
                p->arrival_time,
                p->RUNNING_TIME,
                p->REMAINING_TIME,
                p->WAITING_TIME,
                TA,
                WTA_val);
        fclose(pFile);
    }
    
    if (count < total_process) {
        WTA[count] = WTA_val;
        wait_time[count] = p->WAITING_TIME;
        total_running_time[count] = p->RUNNING_TIME;
        count++;
    }
    
    finished_process++;
}

void Robin_Robin_timestep(int current_time) {
    process next;
    process_Node* temp;
    
    handle_disk_completions(current_time);
    
    if (running_process_index != -1 && pcb[running_process_index].process_state == Running) { // If a process is currently running
        PCB* current_pcb = &pcb[running_process_index];
        
        
        
        int io_time_needed = handle_memory_request(running_process_index, current_time);
        current_pcb->execution_time++;
        total_running_times++;
        
        if (io_time_needed > 0) {
            block_process(running_process_index, current_time, io_time_needed);
            running_process_index = -1;
            quantum_counter = 0;
            return;
        }
        
        current_pcb->REMAINING_TIME--;
        quantum_counter++;
        
        if (current_pcb->REMAINING_TIME <= 0) {
            finish_process(running_process_index, current_time);
            
            for (int i = running_process_index; i < process_count - 1; i++) {
                pcb[i] = pcb[i + 1];
            }
            process_count--;
            
            running_process_index = -1;
            quantum_counter = 0;
            
            if (!is_empty_queue(&READY_QUEUE)) {
                next = dequeue(&READY_QUEUE);
                for (int i = 0; i < process_count; i++) {
                    if (pcb[i].process_id == next.ID) {
                        running_process_index = i;
                        start_process(i, current_time);
                        break;
                    }
                }
            }
            return;
        }
        
        if (quantum_counter >= TIME_QUANTUM) {
            stop_process(running_process_index, current_time);
            
            process temp_process;
            temp_process.ID = current_pcb->process_id;
            temp_process.ARRIVAL_TIME = current_pcb->arrival_time;
            temp_process.RUNNING_TIME = current_pcb->RUNNING_TIME;
            temp_process.PRIORITY = current_pcb->priority;
            temp_process.first_time = 0;
            
            enqueue(&READY_QUEUE, temp_process);
            
            running_process_index = -1;
            quantum_counter = 0;
        }
    }
    
    if (running_process_index == -1 && !is_empty_queue(&READY_QUEUE)) {
        next = dequeue(&READY_QUEUE);
        
        for (int i = 0; i < process_count; i++) {
            if (pcb[i].process_id == next.ID) {
                running_process_index = i;
                start_process(i, current_time);
                break;
            }
        }
    }
  
    temp = READY_QUEUE.front;
    while (temp != NULL) {
        for (int i = 0; i < process_count; i++) {
            if (pcb[i].process_id == temp->Process.ID && pcb[i].process_state == Ready) {
         
                if (pcb[i].START_TIME == -1) {
                    pcb[i].WAITING_TIME++;
                } else {
                    int time_spent_running = pcb[i].RUNNING_TIME - pcb[i].REMAINING_TIME;
                    pcb[i].WAITING_TIME = current_time - pcb[i].arrival_time - time_spent_running;
                }
                if (pcb[i].WAITING_TIME < 0) pcb[i].WAITING_TIME = 0;
                break;
            }
        }
        temp = temp->next;
    }
}

int main(int argc, char * argv[]) {
    int clock_timer = 0;
    int current_time;
    message_buf PROCESS_MESSAGE;
    
    if (argc < 4) {
        printf("Usage: scheduler.out <algorithm> <quantum> <process_count>\n");
        return 1;
    }
    
    pFile = fopen("scheduler.log", "w");
    if (pFile) {
        fprintf(pFile, "#At     time    x     process    y    state    arr    w    total    z    remain    wait    K\n");
        fclose(pFile);
    }
    
    FILE* mem_log = fopen("memory.log", "w");
    if (mem_log) fclose(mem_log);
    
    total_process = atoi(argv[3]);
    
    wait_time = malloc(sizeof(int) * total_process);
    WTA = malloc(sizeof(float) * total_process);
    total_running_time = malloc(sizeof(int) * total_process);
    
    for (int i = 0; i < total_process; i++) {
        wait_time[i] = 0;
        WTA[i] = 0.0;
        total_running_time[i] = 0;
    }
    
    initClk();
    
    init_memory();
    initialize_queue(&READY_QUEUE);
    initialize_priority_queue(&READY_PRIORITY_QUEUE);
    initialize_queue(&BLOCKED_QUEUE);
    
    selected_Algorithm_NUM = atoi(argv[1]);
    TIME_QUANTUM = atoi(argv[2]);
    
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666 | IPC_CREAT);
    if (MESSAGE_ID == -1) {
        printf("Error creating message queue\n");
    }
    
    process_count = 0;
    
    while (1) {
         
        current_time = getClk();
        
        int rec_status = msgrcv(MESSAGE_ID, &PROCESS_MESSAGE, sizeof(process), 2, IPC_NOWAIT);
        if (rec_status != -1) {
            int index = process_count;
            pcb[index].process_id = PROCESS_MESSAGE.p.ID;
            pcb[index].arrival_time = PROCESS_MESSAGE.p.ARRIVAL_TIME;
            pcb[index].RUNNING_TIME = PROCESS_MESSAGE.p.RUNNING_TIME;
            pcb[index].REMAINING_TIME = PROCESS_MESSAGE.p.RUNNING_TIME;
            pcb[index].priority = PROCESS_MESSAGE.p.PRIORITY;
            pcb[index].dependency_id = PROCESS_MESSAGE.p.DEPENDENCY_ID;
            pcb[index].disk_base = PROCESS_MESSAGE.p.disk_base;
            pcb[index].limit = PROCESS_MESSAGE.p.limit;
            pcb[index].num_requests = PROCESS_MESSAGE.p.num_requests;
            pcb[index].process_state = Ready;
            pcb[index].STARTED = 0;
            pcb[index].is_completed = 0;
            pcb[index].WAITING_TIME = 0;
            pcb[index].blocked_time = 0;
            pcb[index].execution_time = 0;
            pcb[index].quantum_remaining = 0;
            pcb[index].START_TIME = -1;
            pcb[index].LAST_EXECUTED_TIME = -1;
            pcb[index].FINISH_TIME = -1;
            pcb[index].process_pid = -1;
            pcb[index].page_table.entries = NULL;
            pcb[index].page_table.physical_page_number = -1;
            
            pcb[index].num_pages = pcb[index].limit;
            if (pcb[index].num_pages <= 0) {
                pcb[index].num_pages = 1;
            }
            
            int max_requests = (PROCESS_MESSAGE.p.num_requests < 100) ? PROCESS_MESSAGE.p.num_requests : 100;
            for (int i = 0; i < max_requests; i++) {
                pcb[index].memory_requests[i] = PROCESS_MESSAGE.p.memory_requests[i];
            }
            
            enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
            process_count++;
            
            total_running_time[running_count] = PROCESS_MESSAGE.p.RUNNING_TIME;
            running_count++;
        }

        if (selected_Algorithm_NUM == 1 && timer != getClk()) { // HPF
            timer = getClk();
            printf("Clock Timer : %d \n",getClk());
            printPriorityQueue(&readyPriorityQueue);
            for (int i = 0; i < pcbCount; i++)
            {
                printf("process%d waiting=%d\n", pcbArray[i]->process_id, pcbArray[i]->WAITING_TIME);
            }
            
            // to stop a process if remaining time = 0
            if (runningPcb != NULL && runningPcb->REMAINING_TIME == 0) {
                kill(runningPcb->process_pid, SIGSTOP);
                runningPcb->FINISH_TIME = timer;
                runningPcb->is_completed = true;

                // printLog(runningPcb, "finished");
                pFile = fopen("scheduler.log", "a");
                if (pFile) {
                    fprintf(pFile,
                            "At time %-5d process %-5d finished arr %-5d total %-5d remain %-5d wait %-5d TA %-5d WTA %.2f\n",
                            timer,
                            runningPcb->process_id,
                            runningPcb->arrival_time,
                            runningPcb->RUNTIME,
                            runningPcb->REMAINING_TIME,
                            runningPcb->WAITING_TIME,
                            runningPcb->FINISH_TIME - runningPcb->arrival_time,
                            (float)(runningPcb->FINISH_TIME - runningPcb->arrival_time)/runningPcb->RUNNING_TIME);
                    fclose(pFile);
                }

                runningPcb = NULL;
                finished_PCB++;
            }

            // if a higher priority process in queue
            if (runningPcb != NULL && !isPriorityQueueEmpty(&readyPriorityQueue)) {
                PCB *temp = peekPriorityFront(&readyPriorityQueue);
                if (temp->priority > runningPcb->priority) {
                    kill(runningPcb->process_pid, SIGUSR2);
                    enqueuePriority(&readyPriorityQueue, runningPcb);

                    // printLog(runningPcb, "stopped");
                    pFile = fopen("scheduler.log", "a");
                    if (pFile) {
                        fprintf(pFile, "At time %-5d process %-5d stopped arr %-5d total %-5d remain %-5d wait %-5d\n",
                                timer,
                                runningPcb->process_id,
                                runningPcb->arrival_time,
                                runningPcb->RUNTIME,
                                runningPcb->REMAINING_TIME,
                                (timer - runningPcb->arrival_time));
                        fclose(pFile);
                    }

                    runningPcb = NULL;
                }
            }
            
            // if no process running, run the highest priority process either continue or start
            if (runningPcb == NULL && !isPriorityQueueEmpty(&readyPriorityQueue)) {
                PCB *p = dequeuePriority(&readyPriorityQueue);
                if (p == NULL) printf("Error in getting process from priority queue\n");
                else {
                    p->LAST_EXECUTED_TIME = timer;
                    if (p->STARTED) {
                        kill(p->process_pid, SIGCONT);
                    }
                    else {
                        int pid = fork();
                        if (pid == 0) {
                            execl("./process.out", "./process.out", p->RUNTIME, NULL);
                            exit(1);
                        }
                        else if (pid > 0) {
                            p->process_pid = pid;
                        }
                        else {
                            printf("error in creating a process\n");
                            exit(1);
                        }
                        p->STARTED = true;
                    }
                    runningPcb = p;

                    pFile = fopen("scheduler.log", "a");
                    if (pFile) {
                        fprintf(pFile, "At time %-5d process %-5d started arr %-5d total %-5d remain %-5d wait %-5d\n",
                                timer,
                                runningPcb->process_id,
                                runningPcb->arrival_time,
                                runningPcb->RUNTIME,
                                runningPcb->REMAINING_TIME,
                                (timer - runningPcb->arrival_time));
                        fclose(pFile);
                    }
                }
            }

            // increase waiting time for all started processes
            if (!isPriorityQueueEmpty(&readyPriorityQueue)) {
                int count = 0;   // just for safety, avoid infinite loops
                int max_iterations = 1000; // adjust based on expected max queue size

                PCB *tempQueue[max_iterations]; // temporary array to hold PCBs
                int tempCount = 0;

                // Dequeue all elements
                while (!isPriorityQueueEmpty(&readyPriorityQueue) && count < max_iterations) {
                    PCB *p = dequeuePriority(&readyPriorityQueue);
                    if (p->STARTED && p->REMAINING_TIME > 0) {
                        p->WAITING_TIME++;
                    }
                    tempQueue[tempCount++] = p; // store temporarily
                    count++;
                }

                // Re-enqueue all back
                for (int i = 0; i < tempCount; i++) {
                    enqueuePriority(&readyPriorityQueue, tempQueue[i]);
                }
            }

            // to decrease remaining time each second
            if (runningPcb != NULL) {
                runningPcb->REMAINING_TIME--;
                runningPcb->RUNNING_TIME++;
            }
        }

        if (selected_Algorithm_NUM == 2 && clock_timer != getClk()) { // SRTN
    clock_timer = getClk();
    printf("Clock Timer : %d \n",getClk());

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

        if (selected_Algorithm_NUM == 3 && clock_timer != getClk()) { // RR
            clock_timer = getClk();
            printf("Clock Timer : %d\n",clock_timer);
            Robin_Robin_timestep(current_time);
        }
        
        if (finished_process == total_process && total_process > 0) {
            if (count > 0) {
                float AVGWAITING = 0;
                float AVGWTA = 0;
                float total_runtime = 0;
                
                for (int i = 0; i < count; i++) {
                    AVGWTA += WTA[i];
                    AVGWAITING += wait_time[i];
                    total_runtime += total_running_time[i];
                }
                
                AVGWAITING /= count;
                AVGWTA /= count;
                
                for (int i = 0; i < count; i++) {
                    std_dev_sqr += pow((WTA[i] - AVGWTA), 2);
                }
                std_dev_sqr /= count;
                float std_dev = sqrt(std_dev_sqr);
                
                float cpu_utilization = ((float)total_running_times / current_time) * 100;
                
                pFile = fopen("scheduler.perf", "w");
                if (pFile) {
                    fprintf(pFile, "CPU utilization = %.2f%%\n", cpu_utilization);
                    fprintf(pFile, "Avg WTA = %.2f\n", AVGWTA);
                    fprintf(pFile, "Avg Waiting = %.2f\n", AVGWAITING);
                    fprintf(pFile, "Std WTA = %.2f\n", std_dev);
                    fclose(pFile);
                }
            }
            
            break;
        }
        
    }
    
    free(wait_time);
    free(WTA);
    free(total_running_time);
    
    close_memory_log();
    
    destroyClk(1);
    return 0;
}