#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>    
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "Processes_DataStructure/process.h"
#include "mmu.h"

#define max 400
typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

typedef enum { Ready, Running, Finished, Waiting, Blocked } state;  

typedef char* string;

struct message_buf {
    long msgtype;
    process p;
} typedef message_buf;

// COMPLETE PCB Structure - Ensure all these fields exist in headers.h

struct PCB_struct {
    int process_id;            // ID of the process (from generator)
    int process_pid;           // PID of forked child
    int priority;              // current priority (can be boosted)
    int dependency_id;         // ID of process it depends on, -1 if none
    state process_state;       // READY, RUNNING, FINISHED, WAITING, BLOCKED
    bool STARTED;              // true if forked at least once
    int REMAINING_TIME;        // runtime left
    int RUNNING_TIME;          // total runtime
    int START_TIME;            // first execution
    int LAST_EXECUTED_TIME;    // last time slice
    int FINISH_TIME;           // termination time
    int arrival_time;          // arrival time (from Process)
    int quantum_remaining;     // optional for round-robin
    bool is_completed;         // true if finished
    int WAITING_TIME;          // total waiting time
    int blocked_time;          // cycles remaining in blocked state
    int execution_time;        // execution time counter for memory requests
    
    // Memory management fields
    ProcessPageTable page_table;
    request memory_requests[100]; // max 100 requests
    int num_requests;          // number of memory requests
    int num_pages;             // number of pages needed
    int disk_base;             // base page on disk
    int limit;                 // limit (number of pages)
} typedef PCB;

typedef struct PCB_node {
    PCB PCB_entry;
    struct PCB_node* next;
} PCB_node;

typedef struct PCB_linked_list {
    PCB_node* head;
    PCB_node* tail;
    int count;
} PCB_linked_list;

// Function declarations
void INITIALIZE_PCB(PCB* pcb);
void INITIALIZE_PCB_Node(PCB_node* pcb_node);
void INITIALIZE_PCB_Linked_List(PCB_linked_list* pcb_list);
void ADD_PCB(PCB_linked_list* pcb_list, PCB pcb_entry);
int Remove_PCB(PCB_linked_list* pcb_list, int process_id);
PCB* get_pcb(PCB* pcb, int process_count, int process_id);
int get_pcb_index(PCB* pcb, int process_count, int process_id);
void remove_pcb(PCB* pcb, int *process_count, int process_id);
int get_count_PCB(PCB_linked_list* pcb_list);
PCB* get_PCB_entry(PCB_linked_list* pcb_list, int process_id);

// PcbPriorityQueue functions
typedef struct PcbNode {
    PCB* pcb;
    struct PcbNode* next;
} PcbNode;

typedef struct {
    PcbNode* front;
    PcbNode* rear;
} PcbPriorityQueue;

void initializePriorityQueue(PcbPriorityQueue* queue);
bool isPriorityQueueEmpty(PcbPriorityQueue* queue);
bool enqueuePriority(PcbPriorityQueue* queue, PCB* pcb);
PCB* dequeuePriority(PcbPriorityQueue* queue);
PCB* peekPriorityFront(PcbPriorityQueue* queue);
void freePriorityQueue(PcbPriorityQueue* queue);
bool removeFromQueue(PcbPriorityQueue* queue, PCB* pcb);
bool updatePriority(PcbPriorityQueue* queue, PCB* pcb, int newPriority);

// Clock functions
int getClk();
void initClk();
void destroyClk(bool terminateAll);

#endif //HEADERS_H