#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H

#include "process.h"

typedef struct process_Node {
    process Process;
    struct process_Node* next;
} process_Node;

typedef struct {
    process_Node* front;
    process_Node* rear;
    int count;
} process_queue;

void initialize_queue(process_queue* q);
int is_empty_queue(process_queue* q);  // Renamed to avoid conflict
void enqueue(process_queue* q, process p);
process dequeue(process_queue* q);
process peek_front(process_queue* q);
int queue_size(process_queue* q);

#endif // PROCESS_QUEUE_H