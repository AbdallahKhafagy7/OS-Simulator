// ***************************abdelrahman******************************* //


#ifndef PROCESS_PRIORITY_QUEUE_H
#define PROCESS_PRIORITY_QUEUE_H

#include "process.h"

// Node definition
typedef struct process_PNode
{
    process Process;
    struct process_PNode* next;
} process_PNode;

// Queue definition
typedef struct process_priority_queue
{
    process_PNode* front;
    process_PNode* rear;
} process_priority_queue;


void initialize_priority_queue(process_priority_queue* Priority_Queue);

/**
 *  1 (true) if the queue is NULL or has no nodes, 0 (false) otherwise.
 */
int is_priority_queue_empty(process_priority_queue* Priority_Queue);

/**
 *  
 *  0 on success, -1 on memory allocation failure, -4 on invalid queue pointer.
 */
int enqueue_priority(process_priority_queue* Priority_Queue, process Process);


/*
enqueue for SRTN
 0 on success, -1 on memory allocation failure, -4 on invalid queue pointer.
*/

int enqueue_priority_SRTN(process_priority_queue* Priority_Queue, process process);
/**
 *  The CALLER is responsible for calling free() on this returned node.
 */

process_PNode* dequeue_priority(process_priority_queue* Priority_Queue);


process_PNode* peek_priority_front(process_priority_queue* Priority_Queue);


void free_priority_queue(process_priority_queue* Priority_Queue);

#endif // PROCESS_PRIORITY_QUEUE_H
