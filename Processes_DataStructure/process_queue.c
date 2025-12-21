#include "process_queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initialize_queue(process_queue* q) {
    if (!q) return;
    q->front = NULL;
    q->rear = NULL;
    q->count = 0;
}

int is_empty_queue(process_queue* q) {
    if (!q) return 1;
    return (q->front == NULL);
}

void enqueue(process_queue* q, process p) {
    if (!q) return;
    
    process_Node* new_node = (process_Node*)malloc(sizeof(process_Node));
    if (!new_node) return;
    
    new_node->Process = p;
    new_node->next = NULL;
    
    if (is_empty_queue(q)) {
        q->front = new_node;
        q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }
    q->count++;
}

process dequeue(process_queue* q) {
    process empty;
    memset(&empty, 0, sizeof(process));
    empty.ID = -1;
    
    if (!q || is_empty_queue(q)) return empty;
    
    process_Node* temp = q->front;
    process p = temp->Process;
    
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    
    free(temp);
    q->count--;
    return p;
}

process peek_front(process_queue* q) {
    process empty;
    memset(&empty, 0, sizeof(process));
    empty.ID = -1;
    
    if (!q || is_empty_queue(q)) return empty;
    
    return q->front->Process;
}

int queue_size(process_queue* q) {
    if (!q) return 0;
    return q->count;
}