#include <string.h>
#include <stdlib.h>

struct process_Node
{
    int process_id;
    struct process_Node* next;
};

 


struct process_queue
{
    struct process_Node* front;
    struct process_Node* rear;
};


void initialize_queue(struct process_queue* q)
{
    q->front = NULL;
    q->rear = NULL;
}

int is_queue_empty(struct process_queue* q)
{
    return (q->front == NULL);
}

int enqueue(struct process_queue* q, int process_id)
{
    if (!q)
        return -4; // Invalid queue pointer

    struct process_Node* new_node = malloc(sizeof(struct process_Node));
    if (!new_node)
        return -1; // Memory allocation failed

    new_node->process_id = process_id;
    new_node->next = NULL;

    if (is_queue_empty(q)) {
        q->front = new_node;
    } else {
        q->rear->next = new_node;
    }
    q->rear = new_node;

    return 0; // Success
}


struct process_Node* dequeue(struct process_queue* q){
    if (!q || is_queue_empty(q))
        return NULL; // Invalid queue pointer or empty queue

    struct process_Node* temp = q->front;
    q->front = q->front->next;

    if (!q->front) // If the queue is now empty
        q->rear = NULL;

    return temp;
}

void free_queue(struct process_queue* q)
{
    if (!q)
        return;

    struct process_Node* current = q->front;
    struct process_Node* next_node;

    while (current) {
        next_node = current->next;
        free(current);
        current = next_node;
    }

    q->front = NULL;
    q->rear = NULL;
}

struct process_Node* peek_front(struct process_queue* q)
{
    if (!q || is_queue_empty(q))
        return NULL; // Invalid queue pointer or empty queue

    return q->front;
}