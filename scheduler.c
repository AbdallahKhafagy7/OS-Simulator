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
int selected_Page_Replacement_NUM = -1;
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


// Function declarations
void start_process(int process_index, int current_time);
void stop_process(int process_index, int current_time);
void block_process(int process_index, int current_time, int io_time);
void finish_process(int process_index, int current_time);
void Robin_Robin_timestep(int current_time);
void handle_disk_completions(int current_time);
int handle_memory_request(int process_index, int current_time, int *frame_out, int *vpage_out, char *rw_out);
void add_disk_operation(int process_id, int virtual_page, int frame_number, char rw_flag, int io_time, int current_time);

// -------------------------

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
        if (disk_queue[i].completion_time <= current_time) {
            int pid = disk_queue[i].process_id;
            int vpage = disk_queue[i].virtual_page;
            int frame = disk_queue[i].frame_number;
            char rw = disk_queue[i].rw_flag;
            
            printf("Completing disk I/O for P%d at time %d\n", pid, current_time);
            
            // NOW complete the page fault (load the page)
            complete_page_fault(pcb, process_count, pid, vpage, frame, rw, current_time);
            
            // Move process from BLOCKED to READY
            process_Node* current_node = BLOCKED_QUEUE.front;
            process_Node* prev = NULL;
            
            while (current_node != NULL) {
                if (current_node->Process.ID == pid) {
                    // Remove from blocked queue
                    if (prev == NULL) {
                        BLOCKED_QUEUE.front = current_node->next;
                    } else {
                        prev->next = current_node->next;
                    }
                    
                    // Add to ready queue
                    enqueue(&READY_QUEUE, current_node->Process);
                    
                    // Update PCB state
                    for (int j = 0; j < process_count; j++) {
                        if (pcb[j].process_id == pid) {
                            pcb[j].process_state = Ready;
                            pcb[j].blocked_time = 0;
                            printf("Process %d moved from BLOCKED to READY\n", pid);
                            break;
                        }
                    }
                    
                    free(current_node);
                    break;
                }
                prev = current_node;
                current_node = current_node->next;
            }
            
            // Remove from disk queue
            for (int j = i; j < disk_queue_size - 1; j++) {
                disk_queue[j] = disk_queue[j + 1];
            }
            disk_queue_size--;
            i--;
        }
    }
}

void add_disk_operation(int process_id, int virtual_page, int frame_number,
                               char rw_flag, int io_time, int current_time) {
    if (disk_queue_size >= 100) {
        printf("Error: Disk queue full!\n");
        return;
    }
    
    disk_queue[disk_queue_size].process_id = process_id;
    disk_queue[disk_queue_size].virtual_page = virtual_page;
    disk_queue[disk_queue_size].frame_number = frame_number;
    disk_queue[disk_queue_size].rw_flag = rw_flag;
    disk_queue[disk_queue_size].completion_time = current_time + io_time;
    disk_queue_size++;
    
    printf("Added disk operation: P%d, vpage=%d, complete at time %d\n",
           process_id, virtual_page, current_time + io_time);
}

int handle_memory_request(int process_index, int current_time, 
                                 int *frame_out, int *vpage_out, char *rw_out) {
    PCB* p = &pcb[process_index];
    
    for (int i = 0; i < p->num_requests; i++) {
        if (p->memory_requests[i].time == p->execution_time) {
            int virtual_address = p->memory_requests[i].address;
            char rw = p->memory_requests[i].rw;
            int virtual_page = get_vpn(virtual_address);
            
            // FIRST page load at process start (execution_time == 0)
            // This happens instantly per project spec
            if (p->execution_time == 0) {
                printf("Process %d: Initial page load - instant (no blocking)\n", p->process_id);
                
                bool is_page_fault = false;
                int frame = -1;
                int disk_time = Request_New(pcb, process_count, p->process_id, 
                                           virtual_page, rw, current_time, 
                                           &frame, &is_page_fault);
                
                if (is_page_fault) {
                    // Complete immediately (no delay for first page)
                    complete_page_fault(pcb, process_count, p->process_id, 
                                       virtual_page, frame, rw, current_time);
                }
                return 0;  // Don't block
            }
            
            // All subsequent memory accesses
            bool is_page_fault = false;
            int frame = -1;
            int disk_time = Request_New(pcb, process_count, p->process_id, 
                                       virtual_page, rw, current_time, 
                                       &frame, &is_page_fault);
            
            if (is_page_fault && disk_time > 0) {
                // Page fault occurred - need to block
                *frame_out = frame;
                *vpage_out = virtual_page;
                *rw_out = rw;
                printf("Process %d: Page fault - blocking for %d cycles\n", 
                       p->process_id, disk_time);
                return disk_time;
            }
            break;
        }
    }
    return 0;
}

void stop_process(int process_index, int current_time) {

    PCB* p = &pcb[process_index];
    
    if (p->process_pid > 0) {
        kill(p->process_pid, SIGSTOP);
    }
    
    p->process_state = Ready;
    
    int time_spent_running = p->RUNNING_TIME - p->REMAINING_TIME;
    p->WAITING_TIME =  current_time - p->arrival_time - p->execution_time - p->total_blocked_time;

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
// Replace the block_process function with this:

void block_process(int process_index, int current_time, int io_time) {
    PCB* p = &pcb[process_index];
    
    if (p->process_pid > 0) {
        kill(p->process_pid, SIGSTOP);
    }
    if (p->REMAINING_TIME <= 0) {
        printf("[ERROR] Trying to block process with no CPU time left!\n");
        finish_process(process_index, current_time);
        return;
    }

    p->process_state = Blocked;
    p->blocked_time = io_time;
    p->total_blocked_time += io_time;  

    
    int time_spent_running = p->RUNNING_TIME - p->REMAINING_TIME;
    p->WAITING_TIME = current_time - p->arrival_time - p->execution_time - p->total_blocked_time;
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
    
    // Disk operation is now added by the caller (Robin_Robin_timestep)
    // Don't add it here
}

// Keep Robin_Robin_timestep as is (it already has the correct call to add_disk_operation)
// Just make sure it's called Robin_Robin_timestep, not Robin_Robin_timestep_fixed

void finish_process(int process_index, int current_time) {
    PCB* p = &pcb[process_index];
    
    p->process_state = Finished;
    p->FINISH_TIME = current_time;
    p->is_completed = 1;
    
  
    int actual_cpu_time = p->RUNNING_TIME - p->REMAINING_TIME;
    p->WAITING_TIME = current_time - p->arrival_time - p->execution_time - p->total_blocked_time;
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
    
    // First, check if any disk operations completed
    handle_disk_completions(current_time);
    
    if (running_process_index != -1 && pcb[running_process_index].process_state == Running) {
        PCB* current_pcb = &pcb[running_process_index];
        
        // FIRST: Check if process has any CPU time left
        if (current_pcb->REMAINING_TIME <= 0) {
            // Process finished - no memory accesses should happen
            finish_process(running_process_index, current_time);
            
            // Remove from PCB array
            for (int i = running_process_index; i < process_count - 1; i++) {
                pcb[i] = pcb[i + 1];
            }
            process_count--;
            
            running_process_index = -1;
            quantum_counter = 0;
            
            // Start next process if available
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
        
        // Process HAS CPU time - check memory request
        int frame_allocated = -1;
        int vpage_faulted = -1;
        char rw_flag = 'r';
        
        int io_time_needed = handle_memory_request(running_process_index, 
                                                   current_time,
                                                   &frame_allocated,
                                                   &vpage_faulted,
                                                   &rw_flag);
        
        if (io_time_needed > 0) {
            // PAGE FAULT - block WITHOUT executing
            block_process(running_process_index, current_time, io_time_needed);
            
            // Add to disk queue
            add_disk_operation(current_pcb->process_id, vpage_faulted,
                              frame_allocated, rw_flag, io_time_needed, 
                              current_time);
            
            running_process_index = -1;
            quantum_counter = 0;
            return;  // CRITICAL: Process didn't execute this cycle
        }
        
        // NO PAGE FAULT - execute normally
        current_pcb->execution_time++;
        current_pcb->REMAINING_TIME--;
        quantum_counter++;
        total_running_times++;
        
        // Check if quantum expired (AFTER successful execution)
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
    
    // If no process running and ready queue has processes, start next
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
}
    
// ALSO FIX start_process to handle ANY initial request time:
void start_process(int process_index, int current_time) {
    PCB* p = &pcb[process_index];
    
    if (!p->STARTED) {
        // Initialize page table BEFORE forking
        if (p->page_table.entries == NULL) {
            if (init_process_page_table(p) != 0) {
                printf("Error: Failed to initialize page table for process %d\n", p->process_id);
                return;
            }
            
            allocate_process_page_table(p);
            
            // Handle FIRST memory request (at execution_time == 0)
            // This happens instantly per project spec
            printf("Process %d: Loading first page without blocking (project assumption)\n", p->process_id);
            
            // Find the request at time 0 (if any)
            for (int i = 0; i < p->num_requests; i++) {
                if (p->memory_requests[i].time == 0) {
                    int virtual_address = p->memory_requests[i].address;
                    int virtual_page = get_vpn(virtual_address);
                    char rw = p->memory_requests[i].rw;
                    
                    // Use the old Request function that loads immediately
                    Request(pcb, process_count, p->process_id, virtual_page, rw, current_time);
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
        p->execution_time = 0;  // Start at 0, will increment when process runs

        p->WAITING_TIME =  current_time - p->arrival_time - p->execution_time - p->total_blocked_time;
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
        // Resume existing process
        if (p->process_pid > 0) {
            kill(p->process_pid, SIGCONT);
        }
        
        int time_spent_running = p->RUNNING_TIME - p->REMAINING_TIME;
        p->WAITING_TIME =  current_time - p->arrival_time - p->execution_time - p->total_blocked_time;
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

// hpf
PCB* pcbArray[max];
int pcbCount = 0;
PCB* runningPcb = NULL;
PcbPriorityQueue readyPriorityQueue;
int timer = 0;
int childFinished = 0;
int finished_PCB = 0;

bool isRunnable(PCB *p) {
    if (p == NULL) return false;

    // already finished → never runnable
    if (p->is_completed)
        return false;

    // no dependency
    if (p->dependency_id == -1)
        return true;

    // search for the depended process
    for (int i = 0; i < pcbCount; i++) {
        if (pcbArray[i]->process_id == p->dependency_id) {
            return pcbArray[i]->is_completed;
        }
    }

    // dependency has not arrived yet
    return false;
}

int main(int argc, char * argv[]) {
    int clock_timer = 0;
    int current_time;
    message_buf PROCESS_MESSAGE;
    
    if (argc < 5) {
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
    
    selected_Page_Replacement_NUM = atoi(argv[4]);
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
    
        // Receive messages
        int rec_status = msgrcv(MESSAGE_ID, &PROCESS_MESSAGE, sizeof(process), 2, IPC_NOWAIT);
        if (rec_status != -1) {
        printf("[DEBUG] Received process %d at clock %d (arrival time in message: %d)\n",
               PROCESS_MESSAGE.p.ID, current_time, PROCESS_MESSAGE.p.ARRIVAL_TIME);
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
            
        
        if (selected_Algorithm_NUM == 3) {
        
            if (running_process_index == -1 && !is_empty_queue(&READY_QUEUE)) {
                process next = dequeue(&READY_QUEUE);
                for (int i = 0; i < process_count; i++) {
                    if (pcb[i].process_id == next.ID) {
                        running_process_index = i;
                        start_process(i, current_time);  
                        break;
                    }
                }
            }
        }
            total_running_time[running_count] = PROCESS_MESSAGE.p.RUNNING_TIME;
            running_count++;
        }

        if (selected_Algorithm_NUM == 1 && timer != getClk()) { // HPF
            timer = getClk();
            printf("Clock Timer: %d\n",timer);
            printPriorityQueue(&readyPriorityQueue);
            
            // to stop a process if remaining time = 0
            if (runningPcb != NULL && runningPcb->REMAINING_TIME == 0) {
                kill(runningPcb->process_pid, SIGSTOP);
                runningPcb->FINISH_TIME = timer;
                runningPcb->is_completed = true;

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
                // Find the highest priority runnable process in the queue
                PCB *bestRunnable = NULL;
                PCB *tempQueue[1000]; // temporary storage
                int tempCount = 0;

                while (!isPriorityQueueEmpty(&readyPriorityQueue)) {
                    PCB *p = dequeuePriority(&readyPriorityQueue);

                    if (isRunnable(p)) {
                        if (bestRunnable == NULL || p->priority > bestRunnable->priority) {
                            bestRunnable = p;
                        }
                    }

                    tempQueue[tempCount++] = p; // keep all processes to restore later
                }

                // restore queue
                for (int i = 0; i < tempCount; i++) {
                    enqueuePriority(&readyPriorityQueue, tempQueue[i]);
                }

                // preempt only if there is a runnable higher-priority process
                if (runningPcb != NULL && bestRunnable != NULL &&
                    bestRunnable->priority > runningPcb->priority) {
                    kill(runningPcb->process_pid, SIGUSR2);
                    enqueuePriority(&readyPriorityQueue, runningPcb);

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
                PCB *selected = NULL;
                PCB *tempQueue[1000];
                int tempCount = 0;

                // find highest priority runnable process
                while (!isPriorityQueueEmpty(&readyPriorityQueue)) {
                    PCB *p = dequeuePriority(&readyPriorityQueue);

                    if (isRunnable(p) && selected == NULL) {
                        selected = p; // pick first runnable (highest priority)
                    } else {
                        tempQueue[tempCount++] = p; // store blocked or lower priority
                    }
                }

                // restore skipped processes
                for (int i = 0; i < tempCount; i++) {
                    enqueuePriority(&readyPriorityQueue, tempQueue[i]);
                }

                // run the selected process if any
                if (runningPcb == NULL && selected != NULL) {
                    PCB *p = selected;
                    p->LAST_EXECUTED_TIME = timer;

                    if (p->STARTED) {
                        kill(p->process_pid, SIGCONT);
                    } else {
                        int pid = fork();
                        if (pid == 0) {
                            execl("./process.out", "./process.out", p->RUNTIME, NULL);
                            exit(1);
                        } else if (pid > 0) {
                            p->process_pid = pid;
                        } else {
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

        if (selected_Algorithm_NUM == 3 && clock_timer != current_time) { // RR
            clock_timer = current_time;
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
    
        if (finished_PCB == total_process) {
            float AVGWAITING = 0;
            float AVGWTA = 0;
            float running = 0;
            float std_dev_sqr = 0;

            for (int i = 0; i < total_process; i++) {
                PCB *p = pcbArray[i];

                int turnaround = p->FINISH_TIME - p->arrival_time;
                float wta = (float)turnaround / p->RUNNING_TIME;

                AVGWAITING += p->WAITING_TIME;
                AVGWTA += wta;
                running += p->RUNNING_TIME;
            }

            AVGWAITING /= total_process;
            AVGWTA /= total_process;

            for (int i = 0; i < total_process; i++) {
                PCB *p = pcbArray[i];
                float wta = (float)(p->FINISH_TIME - p->arrival_time) / p->RUNNING_TIME;
                std_dev_sqr += pow(wta - AVGWTA, 2);
            }
            float std_dev = sqrt(std_dev_sqr / total_process);

            pFile = fopen("scheduler.perf", "w");
            if (pFile) {
                fprintf(pFile,
                    "CPU utilization = %.2f%%\nAvg WTA = %.2f\nAvg Waiting = %.2f\nStd WTA = %.2f\n",
                    (running / getClk()) * 100,
                    AVGWTA,
                    AVGWAITING,
                    std_dev
                );
                fclose(pFile);
                printf("\nPerformance File Has Been Generated!\n");
            }

            finished_PCB = 0;
            total_process = -1;
        }
    }
    
    free(wait_time);
    free(WTA);
    free(total_running_time);
    
    close_memory_log();
    
    destroyClk(1);
    return 0;
}