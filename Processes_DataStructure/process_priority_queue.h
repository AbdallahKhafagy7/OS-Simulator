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

/**
 *  Initializes a priority queue to an empty state.
 */
void initialize_priority_queue(process_priority_queue* Priority_Queue);

/**
 *  Checks if the priority queue is empty.
 *  1 (true) if the queue is NULL or has no nodes, 0 (false) otherwise.
 */
int is_priority_queue_empty(process_priority_queue* Priority_Queue);

/**
 *  Enqueues a process based on its priority (lower number = higher priority).
 *  Process The process to add.
 *  0 on success, -1 on memory allocation failure, -4 on invalid queue pointer.
 */
int enqueue_priority(process_priority_queue* Priority_Queue, process Process);

/**
 *  Dequeues the highest-priority process (from the front) from the queue.
 *  A pointer to the dequeued process_PNode.
 *  The CALLER is responsible for calling free() on this returned node.
 */
process_PNode* dequeue_priority(process_priority_queue* Priority_Queue);

/**
 *  Peeks at the highest-priority process without removing it.
 *  A pointer to the front process_PNode, or NULL if the queue is empty.
 */
process_PNode* peek_priority_front(process_priority_queue* Priority_Queue);

/**
 *  Frees all nodes remaining in the priority queue and resets it.
 */
void free_priority_queue(process_priority_queue* Priority_Queue);

#endif // PROCESS_PRIORITY_QUEUE_H
