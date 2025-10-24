#include <stdlib.h>


struct process_priority_queue
{
    struct process_PNode* front;
    struct process_PNode* rear;
};

struct process_PNode
{
    int process_id;
    int priority;
    struct process_PNode* next;
};

void initialize_priority_queue(struct process_priority_queue* pq)
{
    pq->front = NULL;
    pq->rear = NULL;
}
int is_priority_queue_empty(struct process_priority_queue* pq)
{
    return (pq->front == NULL);
}

int enqueue_priority(struct process_priority_queue* pq, int process_id, int priority) // lower priority number means higher priority
{
    if (!pq)
        return -4; // Invalid queue pointer
    struct process_PNode* new_node = malloc(sizeof(struct process_PNode));
    if (!new_node)
        return -1; // Memory allocation failed
    new_node->process_id = process_id;
    new_node->priority = priority;
    new_node->next = NULL;

    if (is_priority_queue_empty(pq)) {
        pq->front = new_node;
        pq->rear = new_node;
        return 0; // Success
    }
    struct process_PNode* current = pq->front;
    struct process_PNode* previous = NULL;
    while (current != NULL && (current->priority < priority)) {
        previous = current;
        current = current->next;
    }
    if (!previous){
        new_node->next = pq->front;
        pq->front = new_node;
    } else {
        previous->next = new_node;
        new_node->next = current;
        if (current == NULL) {
            pq->rear = new_node;
        }
    }
}

struct process_PNode* dequeue_priority(struct process_priority_queue* pq){
    if (!pq || is_priority_queue_empty(pq))
        return NULL; // Invalid queue pointer or empty queue

    struct process_PNode* temp = pq->front;
    pq->front = pq->front->next;

    if (!pq->front) // If the queue is now empty
        pq->rear = NULL;

    return temp;
}

struct process_PNode* peek_priority_front(struct process_priority_queue* pq)
{
    if (!pq || is_priority_queue_empty(pq))
        return NULL; // Invalid queue pointer or empty queue

    return pq->front;
}


void free_priority_queue(struct process_priority_queue* pq)
{
    if (!pq)
        return;

    struct process_PNode* current = pq->front;
    struct process_PNode* next_node;

    while (current) {
        next_node = current->next;
        free(current);
        current = next_node;
    }

    pq->front = NULL;
    pq->rear = NULL;
}