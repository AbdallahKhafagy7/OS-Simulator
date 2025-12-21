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
#include "memory.h"

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

typedef struct {
    int process_id;
    int completion_time;
    int virtual_page;
    int io_time_remaining;
    int io_time_elapsed;
} DiskOperation;
// Complete PCB Structure
struct PCB_struct {
    int process_id;
    int process_pid;
    int priority;
    int dependency_id;
    state process_state;
    bool STARTED;
    int REMAINING_TIME;
    int RUNNING_TIME;
    int START_TIME;
    int LAST_EXECUTED_TIME;
    int FINISH_TIME;
    int arrival_time;
    int quantum_remaining;
    bool is_completed;
    int WAITING_TIME;
    int blocked_time;
    int execution_time;
    
    // Memory management fields
    ProcessPageTable page_table;
    request memory_requests[100];
    int num_requests;
    int num_pages;
    int disk_base;
    int limit;
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
int get_count_PCB(PCB_linked_list* pcb_list);
PCB* get_PCB_entry(PCB_linked_list* pcb_list, int process_id);
PCB* get_pcb(PCB* pcb_array, int process_count, int process_id);
int get_pcb_index(PCB* pcb_array, int process_count, int process_id);
void remove_pcb(PCB* pcb_array, int *process_count, int process_id);

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

// MMU helper functions
static inline int get_vpn(int virtual_address) {
    return virtual_address >> OFFSET_BITS;
}

static inline int get_offset(int virtual_address) {
    return virtual_address & (PAGE_SIZE - 1);
}

static inline int get_physical_address(int physical_page, int offset) {
    return (physical_page << OFFSET_BITS) | offset;
}
// Add to headers.h
PCB* get_pcb(PCB* pcb_array, int process_count, int process_id);

#endif