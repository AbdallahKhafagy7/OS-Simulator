// ***************************abdelrahman tarek *********************************//

#include <stdlib.h> // For malloc() and free()
#include <stddef.h> // For NULL
#include "Process_queue.h"

void initialize_queue(process_queue* queue)
{
    if (!queue) return; // Defensive check added
    queue->front = NULL;
    queue->rear = NULL;
}

// Check for NULL queue added for safety
int is_queue_empty(process_queue* queue)
{
    if (!queue) return 1;
    return (queue->front == NULL);
}

int enqueue(process_queue* queue, process Process)
{
    if (!queue)
        return -4; // Invalid queue pointer

    process_Node* new_node = malloc(sizeof(process_Node));
    if (!new_node)
        return -1; // Memory allocation failed

    new_node->Process = Process;
    new_node->next = NULL;

    if (is_queue_empty(queue)) {
        queue->front = new_node;
    } else {
        queue->rear->next = new_node;
    }
    
    // The new node is always the new rear
    queue->rear = new_node; 

    return 0; // Success
}


process_Node* dequeue(process_queue* queue){
    // Use the fixed is_queue_empty check
    if (is_queue_empty(queue))
        return NULL; // Invalid queue pointer or empty queue

    process_Node* temp = queue->front;
    queue->front = queue->front->next;

    if (!queue->front) // If the queue is now empty
        queue->rear = NULL;

    temp->next = NULL; // Isolate the node
    return temp; // ** CALLER MUST FREE THIS NODE **
}

void free_queue(process_queue* queue)
{
    if (!queue)
        return;

    process_Node* current = queue->front;
    process_Node* next_node;

    while (current) {
        next_node = current->next;
        free(current); // Free the node
        current = next_node;
    }

    queue->front = NULL;
    queue->rear = NULL;
}

process_Node* peek_front(process_queue* queue)
{
    // Use the fixed is_queue_empty check
    if (is_queue_empty(queue))
        return NULL; // Invalid queue pointer or empty queue

    return queue->front;
}
