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


bool is_priority_queue_empty(process_priority_queue* Priority_Queue);


bool enqueue_priority(process_priority_queue* Priority_Queue, process Process);


bool enqueue_priority_SRTN(process_priority_queue* Priority_Queue, process process);


process dequeue_priority(process_priority_queue* Priority_Queue);


process peek_priority_front(process_priority_queue* Priority_Queue);


void free_priority_queue(process_priority_queue* Priority_Queue);

#endif // PROCESS_PRIORITY_QUEUE_H
