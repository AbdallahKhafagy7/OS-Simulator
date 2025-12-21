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

// Queues
process_queue READY_QUEUE;
process_queue BLOCKED_QUEUE;
int TIME_QUANTUM;
int current_clock = 0;
int running_process_index = -1;
int process_count = 0;
int total_process = 0;

// RR specific
int quantum_counter = 0;

// Disk operations
typedef struct {
    int process_id;
    int completion_time;
    int virtual_page;
    int io_time_remaining;
    int io_time_elapsed;
} DiskOperation;

DiskOperation disk_queue[100];
int disk_queue_size = 0;

/*---------------------------------Helper Functions--------------------------------*/
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

/*---------------------------------Disk Operations--------------------------------*/
void handle_disk_completions(int current_time) {
    for (int i = 0; i < disk_queue_size; i++) {
        if (disk_queue[i].completion_time <= current_time && disk_queue[i].completion_time > 0) {
            int pid = disk_queue[i].process_id;
            
            // Find and move process from blocked to ready
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

/*---------------------------------Memory Request Handling--------------------------------*/
int handle_memory_request(int process_index, int current_time) {
    PCB* p = &pcb[process_index];
    
    // Check for memory requests at current execution time
    for (int i = 0; i < p->num_requests; i++) {
        if (p->memory_requests[i].time == p->execution_time) {
            int virtual_address = p->memory_requests[i].address;
            char rw = p->memory_requests[i].rw;
            int virtual_page = get_vpn(virtual_address);
            
            // SPECIAL CASE: First memory request (time=1) doesn't block
            // According to project: "allocation and first initial loading take no time"
            if (p->execution_time == 1) {
                printf("Process %d: First memory request at time %d - not blocking\n", 
                       p->process_id, p->execution_time);
                // Still call MMU to load page, but don't block
                int io_time = Request(pcb, process_count, p->process_id, virtual_page, rw, current_time);
                // Don't return io_time - process continues execution
                return 0;
            }
            
            // Call MMU to handle request
            int io_time = Request(pcb, process_count, p->process_id, virtual_page, rw, current_time);
            
            if (io_time > 0) {
                return io_time;
            }
            break;
        }
    }
    return 0;
}
/*---------------------------------Process Management--------------------------------*/
void start_process(int process_index, int current_time) {
    PCB* p = &pcb[process_index];
    
    if (!p->STARTED) {
        // Initialize page table if not already done
        if (p->page_table.entries == NULL) {
            if (init_process_page_table(p) != 0) {
                printf("Error: Failed to initialize page table for process %d\n", p->process_id);
                return;
            }
            
            // Allocate page table (no time cost per project spec)
            allocate_process_page_table(p);
            
            // Load first page immediately (no time cost per project spec)
            // Project says: "allocation and first initial loading take no time"
            // So we don't block for first page
            printf("Process %d: Loading first page without blocking (project assumption)\n", p->process_id);
            
            // Process any memory request at time 0
            for (int i = 0; i < p->num_requests; i++) {
                if (p->memory_requests[i].time == 0) {
                    int virtual_address = p->memory_requests[i].address;
                    int virtual_page = get_vpn(virtual_address);
                    char rw = p->memory_requests[i].rw;
                    
                    // Handle first request without blocking
                    int io_time = Request(pcb, process_count, p->process_id, virtual_page, rw, current_time);
                    if (io_time > 0) {
                        // First page fault - load immediately without blocking
                        printf("Process %d: First page fault occurred but not blocking (project spec)\n", p->process_id);
                        // Still don't block - project says no time for first load
                    }
                    break;
                }
            }
        }
        
        // Fork process
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
        
        // Calculate waiting time up to this point
        // Wait time = current time - arrival time (since no execution yet)
        p->WAITING_TIME = current_time - p->arrival_time;
        if (p->WAITING_TIME < 0) p->WAITING_TIME = 0;
        
        // Log start
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
        // Resume process
        if (p->process_pid > 0) {
            kill(p->process_pid, SIGCONT);
        }
        
        // Update waiting time: current time - arrival time - time spent running
        int time_spent_running = p->RUNNING_TIME - p->REMAINING_TIME;
        p->WAITING_TIME = current_time - p->arrival_time - time_spent_running;
        if (p->WAITING_TIME < 0) p->WAITING_TIME = 0;
        
        // Log resume
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

void stop_process(int process_index, int current_time, const char* reason) {
    PCB* p = &pcb[process_index];
    
    if (p->process_pid > 0) {
        kill(p->process_pid, SIGSTOP);
    }
    
    p->process_state = Ready;
    
    // Update waiting time for the stop
    int time_spent_running = p->RUNNING_TIME - p->REMAINING_TIME;
    p->WAITING_TIME = current_time - p->arrival_time - time_spent_running;
    if (p->WAITING_TIME < 0) p->WAITING_TIME = 0;
    
    // Log stop
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

void block_process(int process_index, int current_time, int io_time, const char* reason) {
    PCB* p = &pcb[process_index];
    
    if (p->process_pid > 0) {
        kill(p->process_pid, SIGSTOP);
    }
    
    p->process_state = Blocked;
    p->blocked_time = io_time;
    
    // Update waiting time before blocking
    int time_spent_running = p->RUNNING_TIME - p->REMAINING_TIME;
    p->WAITING_TIME = current_time - p->arrival_time - time_spent_running;
    if (p->WAITING_TIME < 0) p->WAITING_TIME = 0;
    
    // Create process struct for blocked queue
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
    
    // Log block
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
    
    // Find the virtual page that caused the fault
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
    
    // Calculate final waiting time
    // Wait time = total time in system - actual CPU time used
    int actual_cpu_time = p->RUNNING_TIME - p->REMAINING_TIME;
    p->WAITING_TIME = current_time - p->arrival_time - actual_cpu_time;
    if (p->WAITING_TIME < 0) p->WAITING_TIME = 0;
    
    // Free memory resources
    free_process_pages(p->process_id, p);
    
    // Kill process
    if (p->process_pid > 0) {
        kill(p->process_pid, SIGUSR2);
    }
    
    // Calculate turnaround time statistics
    int TA = p->FINISH_TIME - p->arrival_time;
    float WTA_val = 0.0;
    if (p->RUNNING_TIME > 0) {
        WTA_val = (float)TA / p->RUNNING_TIME;
    }
    
    // Log finish
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
    
    // Store statistics
    if (count < total_process) {
        WTA[count] = WTA_val;
        wait_time[count] = p->WAITING_TIME;
        total_running_time[count] = p->RUNNING_TIME;
        count++;
    }
    
    finished_process++;
}

/*---------------------------------Round Robin with MMU--------------------------------*/
void rr_mmu_tick(int current_time) {
    process next;
    process_Node* temp;
    
    // Handle disk completions first
    handle_disk_completions(current_time);
    
    // If a process is running
    if (running_process_index != -1 && pcb[running_process_index].process_state == Running) {
        PCB* current_pcb = &pcb[running_process_index];
        
        // Increment execution time
        current_pcb->execution_time++;
        total_running_times++;
        
        // Check for memory requests
        int io_time_needed = handle_memory_request(running_process_index, current_time);
        
        if (io_time_needed > 0) {
            // Block process due to page fault
            block_process(running_process_index, current_time, io_time_needed, "page fault");
            running_process_index = -1;
            quantum_counter = 0;
            return;
        }
        
        // Decrement remaining time
        current_pcb->REMAINING_TIME--;
        quantum_counter++;
        
        // Check if process finished
        if (current_pcb->REMAINING_TIME <= 0) {
            finish_process(running_process_index, current_time);
            
            // Remove from process array
            for (int i = running_process_index; i < process_count - 1; i++) {
                pcb[i] = pcb[i + 1];
            }
            process_count--;
            
            running_process_index = -1;
            quantum_counter = 0;
            
            // Schedule next if available
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
        
        // Check quantum expiry
        if (quantum_counter >= TIME_QUANTUM) {
            stop_process(running_process_index, current_time, "quantum expiry");
            
            // Move back to ready queue
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
    
    // If no process running and ready queue not empty
    if (running_process_index == -1 && !is_empty_queue(&READY_QUEUE)) {
        next = dequeue(&READY_QUEUE);
        
        // Find PCB
        for (int i = 0; i < process_count; i++) {
            if (pcb[i].process_id == next.ID) {
                running_process_index = i;
                start_process(i, current_time);
                break;
            }
        }
    }
    
    // Update waiting times for processes in ready queue
    // Only update for processes that are actually in Ready state
    temp = READY_QUEUE.front;
    while (temp != NULL) {
        for (int i = 0; i < process_count; i++) {
            if (pcb[i].process_id == temp->Process.ID && pcb[i].process_state == Ready) {
                // Only increment waiting time if process is ready but not running
                // And hasn't started execution yet
                if (pcb[i].START_TIME == -1) {
                    pcb[i].WAITING_TIME++;
                } else {
                    // If started, wait time = current time - arrival - time spent running
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

/*---------------------------------Main Function--------------------------------*/
int main(int argc, char * argv[]) {
    int clock_timer = 0;
    int current_time;
    message_buf PROCESS_MESSAGE;
    
    // Validate arguments
    if (argc < 4) {
        printf("Usage: scheduler.out <algorithm> <quantum> <process_count>\n");
        return 1;
    }
    
    // Initialize log files
    pFile = fopen("scheduler.log", "w");
    if (pFile) {
        fprintf(pFile, "#At     time    x     process    y    state    arr    w    total    z    remain    wait    K\n");
        fclose(pFile);
    }
    
    // Clear memory.log
    FILE* mem_log = fopen("memory.log", "w");
    if (mem_log) fclose(mem_log);
    
    total_process = atoi(argv[3]);
    
    // Allocate statistics arrays
    wait_time = malloc(sizeof(int) * total_process);
    WTA = malloc(sizeof(float) * total_process);
    total_running_time = malloc(sizeof(int) * total_process);
    
    // Initialize arrays
    for (int i = 0; i < total_process; i++) {
        wait_time[i] = 0;
        WTA[i] = 0.0;
        total_running_time[i] = 0;
    }
    
    initClk();
    
    // Initialize systems
    init_memory();
    initialize_queue(&READY_QUEUE);
    initialize_queue(&BLOCKED_QUEUE);
    
    selected_Algorithm_NUM = atoi(argv[1]);
    TIME_QUANTUM = atoi(argv[2]);
    
    // Setup message queue
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666 | IPC_CREAT);
    if (MESSAGE_ID == -1) {
        printf("Error creating message queue\n");
    }
    
    process_count = 0;
    
    // Main loop
    while (1) {
        current_time = getClk();
        
        // Check for new arrivals
        int rec_status = msgrcv(MESSAGE_ID, &PROCESS_MESSAGE, sizeof(process), 2, IPC_NOWAIT);
        if (rec_status != -1) {
            // Create PCB
            int idx = process_count;
            pcb[idx].process_id = PROCESS_MESSAGE.p.ID;
            pcb[idx].arrival_time = PROCESS_MESSAGE.p.ARRIVAL_TIME;
            pcb[idx].RUNNING_TIME = PROCESS_MESSAGE.p.RUNNING_TIME;
            pcb[idx].REMAINING_TIME = PROCESS_MESSAGE.p.RUNNING_TIME;
            pcb[idx].priority = PROCESS_MESSAGE.p.PRIORITY;
            pcb[idx].dependency_id = PROCESS_MESSAGE.p.DEPENDENCY_ID;
            pcb[idx].disk_base = PROCESS_MESSAGE.p.disk_base;
            pcb[idx].limit = PROCESS_MESSAGE.p.limit;
            pcb[idx].num_requests = PROCESS_MESSAGE.p.num_requests;
            pcb[idx].process_state = Ready;
            pcb[idx].STARTED = 0;
            pcb[idx].is_completed = 0;
            pcb[idx].WAITING_TIME = 0;
            pcb[idx].blocked_time = 0;
            pcb[idx].execution_time = 0;
            pcb[idx].quantum_remaining = 0;
            pcb[idx].START_TIME = -1;
            pcb[idx].LAST_EXECUTED_TIME = -1;
            pcb[idx].FINISH_TIME = -1;
            pcb[idx].process_pid = -1;
            pcb[idx].page_table.entries = NULL;
            pcb[idx].page_table.physical_page_number = -1;
            
            // Calculate number of pages needed
            pcb[idx].num_pages = pcb[idx].limit;
            if (pcb[idx].num_pages <= 0) {
                pcb[idx].num_pages = 1;
            }
            
            // Copy memory requests
            int max_requests = (PROCESS_MESSAGE.p.num_requests < 100) ? PROCESS_MESSAGE.p.num_requests : 100;
            for (int i = 0; i < max_requests; i++) {
                pcb[idx].memory_requests[i] = PROCESS_MESSAGE.p.memory_requests[i];
            }
            
            // Add to ready queue
            enqueue(&READY_QUEUE, PROCESS_MESSAGE.p);
            process_count++;
            
            total_running_time[running_count] = PROCESS_MESSAGE.p.RUNNING_TIME;
            running_count++;
        }
        
        // Run appropriate scheduler
        if (selected_Algorithm_NUM == 3) { // RR
            rr_mmu_tick(current_time);
        }
        
        // Check if all processes finished
        if (finished_process == total_process && total_process > 0) {
            // Generate performance report
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
                
                // Calculate standard deviation
                for (int i = 0; i < count; i++) {
                    std_dev_sqr += pow((WTA[i] - AVGWTA), 2);
                }
                std_dev_sqr /= count;
                float std_dev = sqrt(std_dev_sqr);
                
                // Calculate CPU utilization
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
        
        // Wait for next clock tick
        if (clock_timer == current_time) {
            usleep(100000);
        }
        clock_timer = current_time;
    }
    
    // Cleanup
    free(wait_time);
    free(WTA);
    free(total_running_time);
    
    // Close memory log
    close_memory_log();
    
    destroyClk(1);
    return 0;
}