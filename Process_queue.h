// **********************************abdelrahman tarek *********************************//
#include <string.h>
#include <stdlib.h>
#include "process.h"
typedef struct process_Node
{

    process Process;
    struct process_Node* next;
}process_Node;

 


typedef struct process_queue
{
    process_Node* front;
    process_Node* rear;
}process_queue;


void initialize_queue(process_queue* queue)
{
    queue->front = NULL;
    queue->rear = NULL;
}

int is_queue_empty(process_queue* queue)
{
    return (queue->front == NULL);
}

int enqueue(process_queue* queue, process process)
{
    if (!queue)
        return -4; // Invalid queue pointer

    process_Node* new_node = malloc(sizeof(process_Node));
    if (!new_node)
        return -1; // Memory allocation failed

    new_node->Process = process;
    new_node->next = NULL;

    if (is_queue_empty(queue)) {
        queue->front = new_node;
    } else {
        queue->rear->next = new_node;
    }
    queue->rear = new_node;

    return 0; // Success
}


process_Node* dequeue(process_queue* queue){
    if (!queue || is_queue_empty(queue))
        return NULL; // Invalid queue pointer or empty queue

    process_Node* temp = queue->front;
    queue->front = queue->front->next;

    if (!queue->front) // If the queue is now empty
        queue->rear = NULL;

    return temp;
}

void free_queue(   process_queue* queue)
{
    if (!queue)
        return;

    process_Node* current = queue->front;
    process_Node* next_node;

    while (current) {
        next_node = current->next;
        free(current);
        current = next_node;
    }

    queue->front = NULL;
    queue->rear = NULL;
}

process_Node* peek_front(process_queue* queue)
{
    if (!queue || is_queue_empty(queue))
        return NULL; // Invalid queue pointer or empty queue

    return queue->front;
}