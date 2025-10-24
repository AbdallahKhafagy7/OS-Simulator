// ***************************abdelrahman******************************* //


#ifndef PROCESS_PRIORITY_QUEUE_H
#define PROCESS_PRIORITY_QUEUE_H

#include "process.h" // Assumes process.h defines the 'process' struct

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
 *  Priority_Queue Pointer to the priority queue.
 */
void initialize_priority_queue(process_priority_queue* Priority_Queue);

/**
 *  Checks if the priority queue is empty.
 *  Priority_Queue Pointer to the priority queue.
 * @return 1 (true) if the queue is NULL or has no nodes, 0 (false) otherwise.
 */
int is_priority_queue_empty(process_priority_queue* Priority_Queue);

/**
 *  Enqueues a process based on its priority (lower number = higher priority).
 *  Priority_Queue Pointer to the priority queue.
 *  Process The process to add.
 * @return 0 on success, -1 on memory allocation failure, -4 on invalid queue pointer.
 */
int enqueue_priority(process_priority_queue* Priority_Queue, process Process);

/**
 *  Dequeues the highest-priority process (from the front) from the queue.
 *  Priority_Queue Pointer to the priority queue.
 * @return A pointer to the dequeued process_PNode. 
 * **WARNING:** The CALLER is responsible for calling free() on this returned node
 * to prevent a memory leak.
 * Returns NULL if the queue is empty or invalid.
 */
process_PNode* dequeue_priority(process_priority_queue* Priority_Queue);

/**
 *  Peeks at the highest-priority process without removing it.
 *  Priority_Queue Pointer to the priority queue.
 * @return A pointer to the front process_PNode, or NULL if the queue is empty.
 */
process_PNode* peek_priority_front(process_priority_queue* Priority_Queue);

/**
 *  Frees all nodes remaining in the priority queue and resets it.
 *  Priority_Queue Pointer to the priority queue.
 */
void free_priority_queue(process_priority_queue* Priority_Queue);

#endif // PROCESS_PRIORITY_QUEUE_H
