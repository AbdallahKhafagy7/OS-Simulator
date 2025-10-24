// ***************************abdelrahman******************************* //
#include <stdlib.h>
#include "headers.h"
#include "process_generator.h"

struct process_PNode
{
    process p;
    struct process_PNode* next;
};

struct process_priority_queue
{
    struct process_PNode* front;
    struct process_PNode* rear;
};

void initialize_priority_queue(struct process_priority_queue* pq)
{
    if (!pq) return;
    pq->front = NULL;
    pq->rear = NULL;
}
int is_priority_queue_empty(struct process_priority_queue* pq)
{
    return (pq && pq->front == NULL);
}

int enqueue_priority(struct process_priority_queue* pq, process p) // lower priority number means higher priority
{
    if (!pq)
        return -4; // Invalid queue pointer
    struct process_PNode* new_node = malloc(sizeof(struct process_PNode));
    if (!new_node)
        return -1; // Memory allocation failed
    new_node->p = p;
    new_node->next = NULL;

    if (is_priority_queue_empty(pq)) {
        pq->front = new_node;
        pq->rear = new_node;
        return 0; // Success
    }
    struct process_PNode* current = pq->front;
    struct process_PNode* previous = NULL;
    /* advance past nodes with higher or equal priority (lower number = higher priority),
       so stop when we find a node with a larger priority number (lower priority) */
    while (current != NULL && current->p.priority <= p.priority) {
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
    return 0;
}

struct process_PNode* dequeue_priority(struct process_priority_queue* pq){
    if (!pq || is_priority_queue_empty(pq))
        return NULL; // Invalid queue pointer or empty queue

    struct process_PNode* temp = pq->front; // temp is the head
    pq->front = pq->front->next; // advance the head

    if (!pq->front) // If the queue is now empty
        pq->rear = NULL; // make the rear NULL as well

    temp->next = NULL;
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