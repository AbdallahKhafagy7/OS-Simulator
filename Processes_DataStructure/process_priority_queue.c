// ***************************abdelrahman******************************* //


#include "process_priority_queue.h"
#include <stdlib.h> // For malloc() and free()
#include <stddef.h> // For NULL

void initialize_priority_queue(process_priority_queue* Priority_Queue)
{
    if (!Priority_Queue) return;
    Priority_Queue->front = NULL;
    Priority_Queue->rear = NULL;
}

int is_priority_queue_empty(process_priority_queue* Priority_Queue)
{
    if (!Priority_Queue)
        return 1; // A NULL queue is empty
        
    return (Priority_Queue->front == NULL);
}

int enqueue_priority(process_priority_queue* Priority_Queue, process Process) 
{
    if (!Priority_Queue)
        return -4; // Invalid queue pointer

    process_PNode* new_node = malloc(sizeof(process_PNode));
    if (!new_node)
        return -1; // Memory allocation failed

    new_node->Process = Process;
    new_node->next = NULL;

    if (is_priority_queue_empty(Priority_Queue)) {
        Priority_Queue->front = new_node;
        Priority_Queue->rear = new_node;
        return 0; // Success
    }

    process_PNode* current = Priority_Queue->front;
    process_PNode* previous = NULL;

    /* advance past nodes with higher or equal priority (lower number = higher priority),
       so stop when we find a node with a larger priority number (lower priority) */
    while (current != NULL && current->Process.PRIORITY <= Process.PRIORITY) {
        previous = current;
        current = current->next;
    }

    if (!previous){ // Insert at the front
        new_node->next = Priority_Queue->front;
        Priority_Queue->front = new_node;
    } else { // Insert in the middle or at the end
        previous->next = new_node;
        new_node->next = current;
        if (current == NULL) { // This was an insertion at the end
            Priority_Queue->rear = new_node;
        }
    }
    return 0;
}
int enqueue_priority_SRTN(process_priority_queue* Priority_Queue, process process) // enqueue based on shortest remaining time next (SRTN)
{
   if (!Priority_Queue){
    return -4; // Invalid queue pointer
   }

   process_PNode* new_node = malloc(sizeof(process_PNode));
   if (!new_node)
       return -1; // Memory allocation failed

   new_node->Process = process;
   new_node->next = NULL;
   int priority = process.RUNNING_TIME;

   if (is_priority_queue_empty(Priority_Queue)) {
       Priority_Queue->front = new_node;
       Priority_Queue->rear = new_node;
       return 0; // Success
   }
   
   process_PNode* current = Priority_Queue->front;
   process_PNode* previous = NULL;

   while (current != NULL && current->Process.RUNNING_TIME <= priority) {
       previous = current;
       current = current->next;
   }
  
   if (!previous){ // Insert at the front
       new_node->next = Priority_Queue->front;
       Priority_Queue->front = new_node;
   } else { // Insert in the middle or at the end
       previous->next = new_node;
       new_node->next = current;
       if (current == NULL) { // This was an insertion at the end
           Priority_Queue->rear = new_node;
       }
   }
   return 0;
}



process_PNode* dequeue_priority(process_priority_queue* Priority_Queue){
    if (is_priority_queue_empty(Priority_Queue))
        return NULL; // Invalid queue pointer or empty queue

    process_PNode* temp = Priority_Queue->front; // temp is the head
    Priority_Queue->front = Priority_Queue->front->next; // advance the head

    if (!Priority_Queue->front) // If the queue is now empty
        Priority_Queue->rear = NULL; // make the rear NULL as well

    temp->next = NULL; // Isolate the node
    return temp; // ** CALLER MUST FREE THIS NODE **
}

process_PNode* peek_priority_front(process_priority_queue* Priority_Queue)
{
    // *** SIMPLIFIED CHECK ***
    if (is_priority_queue_empty(Priority_Queue))
        return NULL; // Invalid queue pointer or empty queue

    return Priority_Queue->front;
}


void free_priority_queue(process_priority_queue* Priority_Queue)
{
    if (!Priority_Queue)
        return;

    process_PNode* current = Priority_Queue->front;
    process_PNode* next_node;

    while (current) {
        next_node = current->next;
        free(current); // Free the node
        current = next_node;
    }

    Priority_Queue->front = NULL;
    Priority_Queue->rear = NULL;
}
