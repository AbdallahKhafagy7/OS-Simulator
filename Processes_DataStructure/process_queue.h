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

void initialize_queue(process_queue* queue);

/**
 *  Checks if the queue is empty.
 *  1 (true) if the queue is NULL or has no nodes, 0 (false) otherwise.
 */
bool is_queue_empty(process_queue* queue);

/**
 *  0 on success, -1 on memory allocation failure, -4 on invalid queue pointer.
 */
bool enqueue(process_queue* queue, process Process);


process dequeue(process_queue* queue);


void free_queue(process_queue* queue);

process peek_front(process_queue* queue);

#endif // PROCESS_QUEUE_H
