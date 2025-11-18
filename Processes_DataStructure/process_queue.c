// ***************************abdelrahman tarek *********************************//

#include <stdlib.h> 
#include <stddef.h> 
#include "process_queue.h"

void initialize_queue(process_queue* queue)
{
    if (!queue) return; 
    queue->front = NULL;
    queue->rear = NULL;
}


bool is_queue_empty(process_queue* queue)
{
    if (!queue) return true;
    if (queue->front == NULL) return true;
    return false;
}

bool enqueue(process_queue* queue, process Process)
{
    if (!queue)
        return false; // Invalid queue pointer

    process_Node* new_node = malloc(sizeof(process_Node));
    if (!new_node)
        return false; // Memory allocation failed

    new_node->Process = Process;
    new_node->next = NULL;

    if (is_queue_empty(queue)) {
        queue->front = new_node;
    } else {
        queue->rear->next = new_node;
    }
    
    // The new node is always the new rear
    queue->rear = new_node; 
    queue->rear->next = queue->front; // Circular link

    return true; // Success
}


process dequeue(process_queue* queue){
    // Use the fixed is_queue_empty check
    if (is_queue_empty(queue))
        return; // Invalid queue pointer or empty queue

    process_Node* temp = queue->front;

    // If there is only one node in the queue
    if (queue->front == queue->rear) {
        queue->front = NULL;
        queue->rear = NULL;
    } else {
        queue->front = queue->front->next;
        queue->rear->next = queue->front; // Maintain circular link
    }
    process Process= temp->Process;

    return Process; // Return the dequeued node
}

void free_queue(process_queue* queue)
{
    if (!queue)
        return;

    if (!queue->front) { // Empty queue
        queue->rear = NULL;
        return;
    }

    process_Node* start = queue->front;
    process_Node* current = start->next;

    // Free all nodes except the start node
    while (current != start) {
        process_Node* next_node = current->next;
        free(current); // Free the node
        current = next_node;
    }

    // Free the start node
    free(start);

    queue->front = NULL;
    queue->rear = NULL;

}

process peek_front(process_queue* queue)
{
    // Use the fixed is_queue_empty check
    if (is_queue_empty(queue))
        return; // Invalid queue pointer or empty queue
    return queue->front->Process;
}
