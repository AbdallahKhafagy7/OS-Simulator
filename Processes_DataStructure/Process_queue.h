// ***************************abdelrahman tarek *********************************//

#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H

#include "process.h" 

// Node definition for the standard queue
typedef struct process_Node
{
    process Process;
    struct process_Node* next;
} process_Node;

// Queue definition
typedef struct process_queue
{
    process_Node* front;
    process_Node* rear;
} process_queue;

/**
 *  Initializes a queue to an empty state.
 *  queue Pointer to the queue.
 */
void initialize_queue(process_queue* queue);

/**
 *  Checks if the queue is empty.
 *  queue Pointer to the queue.
 * @return 1 (true) if the queue is NULL or has no nodes, 0 (false) otherwise.
 */
int is_queue_empty(process_queue* queue);

/**
 *  Enqueues a process to the rear of the queue (FIFO).
 *  queue Pointer to the queue.
 *  Process The process to add.
 * @return 0 on success, -1 on memory allocation failure, -4 on invalid queue pointer.
 */
int enqueue(process_queue* queue, process Process);

/**
 *  Dequeues the process from the front of the queue (FIFO).
 *  queue Pointer to the queue.
 * @return A pointer to the dequeued process_Node.
 * **WARNING:** The CALLER is responsible for calling free() on this returned node
 * to prevent a memory leak.
 * Returns NULL if the queue is empty or invalid.
 */
process_Node* dequeue(process_queue* queue);

/**
 *  Frees all nodes remaining in the queue and resets it.
 *  queue Pointer to the queue.
 */
void free_queue(process_queue* queue);

/**
 *  Peeks at the front process without removing it.
 *  queue Pointer to the queue.
 * @return A pointer to the front process_Node, or NULL if the queue is empty.
 */
process_Node* peek_front(process_queue* queue);

#endif // PROCESS_QUEUE_H
