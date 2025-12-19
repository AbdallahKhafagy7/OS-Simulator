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
#include <stdio.h>
#include <time.h>
#include <unistd.h>
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
// struct 

typedef enum { Ready, Running, Finished, Waiting ,Blocked} state;  

typedef char* string;

struct message_buf{
    long msgtype;
    process p;
}typedef message_buf ;



struct PCB_struct{
    int process_id;            // ID of the process (from generator)
    int process_pid;           // PID of forked child
    int priority;              // current priority (can be boosted)
    int dependency_id;         // ID of process it depends on, -1 if none
    state process_state;       // READY, RUNNING, FINISHED, WAITING
    bool STARTED;              // true if forked at least once
    int REMAINING_TIME;        // runtime left
    int RUNNING_TIME;          // total runtime
    int START_TIME;            // first execution
    int LAST_EXECUTED_TIME;    // last time slice
    int FINISH_TIME;           // termination time
    int arrival_time;          // arrival time (from Process)
    int quantum_remaining;     // optional for round-robin
    bool is_completed;         // true if finished
    int WAITING_TIME;
    int blocked_time;
    ProcessPageTable page_table;
    request memory_requests[1000];
    int num_requests;
    int num_pages;
    int disk_base;
    int limit;             
}typedef PCB;



typedef struct PCB_node
{
    PCB PCB_entry;
    struct PCB_node* next;

}PCB_node;

typedef struct PCB_linked_list
{
    PCB_node* head;
    PCB_node* tail;
    int count;
}PCB_linked_list;


void INITIALIZE_PCB(PCB* pcb);
void INITIALIZE_PCB_Node(PCB_node* pcb_node);

void INITIALIZE_PCB_Linked_List(PCB_linked_list* pcb_list);



void ADD_PCB(PCB_linked_list* pcb_list, PCB pcb_entry);
int Remove_PCB(PCB_linked_list* pcb_list, int process_id);


// PCB get_PCB_index(PCB_linked_list* pcb_list, int process_id){
//     PCB pcb_entry;
//     pcb_entry.process_id=-1;
//     if(pcb_list->head==NULL){
//         return pcb_entry;
//     }

//     PCB_node* current=pcb_list->head;
//     int index=0;

//     while(current!=NULL){
//         if(current->PCB_entry.process_id==process_id){
//             return current->PCB_entry;
//         }
//         current=current->next;
//         index++;
//     }
//     return pcb_entry; // Not found
// }
PCB* get_pcb(PCB*pcb,int process_count,int process_id);
int get_pcb_index(PCB*pcb,int process_count,int process_id);
void remove_pcb(PCB*pcb,int *process_count,int process_id);
int get_count_PCB(PCB_linked_list* pcb_list);
PCB* get_PCB_entry(PCB_linked_list* pcb_list, int process_id);

// -=-=-=-=-=-
typedef struct PcbNode {
    PCB* pcb;
    struct PcbNode* next;
} PcbNode;

typedef struct {
    PcbNode* front;
    PcbNode* rear;
} PcbPriorityQueue;

// Initialize queue
void initializePriorityQueue(PcbPriorityQueue* queue) ;

// Check if queue is empty
bool isPriorityQueueEmpty(PcbPriorityQueue* queue) ;

// Enqueue PCB based on priority (lower number = higher priority)
bool enqueuePriority(PcbPriorityQueue* queue, PCB* pcb) ;

// Dequeue PCB (highest priority, front of queue)
PCB* dequeuePriority(PcbPriorityQueue* queue) ;

// Peek at front PCB
PCB* peekPriorityFront(PcbPriorityQueue* queue) ;

// Free entire queue
void freePriorityQueue(PcbPriorityQueue* queue) ;

bool removeFromQueue(PcbPriorityQueue* queue, PCB* pcb) ;
bool updatePriority(PcbPriorityQueue* queue, PCB* pcb, int newPriority) ;



int getClk()
;


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
;

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
;

#endif //HEADERS_H