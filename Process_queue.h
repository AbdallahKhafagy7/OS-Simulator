// **********************************abdelrahman tarek *********************************//
#include <string.h>
#include <stdlib.h>
#include "process.h"
struct process_Node
{

    process Process;
    struct process_Node* next;
};

 


struct process_queue
{
    struct process_Node* front;
    struct process_Node* rear;
};


void initialize_queue(struct process_queue* queue)
{
    queue->front = NULL;
    queue->rear = NULL;
}

int is_queue_empty(struct process_queue* queue)
{
    return (queue->front == NULL);
}

int enqueue(struct process_queue* queue, process process)
{
    if (!queue)
        return -4; // Invalid queue pointer

    struct process_Node* new_node = malloc(sizeof(struct process_Node));
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


struct process_Node* dequeue(struct process_queue* queue){
    if (!queue || is_queue_empty(queue))
        return NULL; // Invalid queue pointer or empty queue

    struct process_Node* temp = queue->front;
    queue->front = queue->front->next;

    if (!queue->front) // If the queue is now empty
        queue->rear = NULL;

    return temp;
}

void free_queue(struct process_queue* queue)
{
    if (!queue)
        return;

    struct process_Node* current = queue->front;
    struct process_Node* next_node;

    while (current) {
        next_node = current->next;
        free(current);
        current = next_node;
    }

    queue->front = NULL;
    queue->rear = NULL;
}

struct process_Node* peek_front(struct process_queue* queue)
{
    if (!queue || is_queue_empty(queue))
        return NULL; // Invalid queue pointer or empty queue

    return queue->front;
}