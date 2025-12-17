#include "headers.h"
///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================


void INITIALIZE_PCB(PCB* pcb){
    pcb->process_state=Ready;
    pcb->REMAINING_TIME=0;
    pcb->RUNNING_TIME=0;
    pcb->START_TIME=-1;
    pcb->LAST_EXECUTED_TIME=-1;
    pcb->FINISH_TIME=-1;
    pcb->process_pid=-1;
    pcb->dependency_id=0;
    pcb->is_completed=false;
}
void INITIALIZE_PCB_Node(PCB_node* pcb_node){
    INITIALIZE_PCB(&pcb_node->PCB_entry);
    pcb_node->next=NULL;
}

void INITIALIZE_PCB_Linked_List(PCB_linked_list* pcb_list){
    INITIALIZE_PCB_Node(pcb_list->head);
    INITIALIZE_PCB_Node(pcb_list->tail);
    pcb_list->count=0;
}



void ADD_PCB(PCB_linked_list* pcb_list, PCB pcb_entry){
    PCB_node* new_node=(PCB_node*)malloc(sizeof(PCB_node));
    if (!new_node){
        perror("Memory allocation failed for PCB_node\n");
        return;
    }

    INITIALIZE_PCB_Node(new_node);
    new_node->PCB_entry=pcb_entry;

    if(pcb_list->head==NULL){
        pcb_list->head=new_node;
        pcb_list->tail=new_node;
        new_node->next=NULL;
        pcb_list->count++;
        return;
    }else{
        pcb_list->tail->next=new_node;
        pcb_list->tail=new_node;
        pcb_list->tail->next=NULL;
        pcb_list->count++;
        return;
    }
}
int Remove_PCB(PCB_linked_list* pcb_list, int process_id){
    if(pcb_list->head==NULL){
        return -1;
    }

    PCB_node* current=pcb_list->head;
    PCB_node* previous=NULL;

    while(current!=NULL){
        if(current->PCB_entry.process_id==process_id){
            if(previous==NULL){ // if head needs to be removed
                pcb_list->head=current->next;
                if(pcb_list->head==NULL){ // if list became empty
                    pcb_list->tail=NULL;
                }
            }else{
                previous->next=current->next;
                if(current==pcb_list->tail){ // if tail needs to be removed
                    pcb_list->tail=previous;
                    pcb_list->tail->next=NULL;
                }
            }
            free(current);
            pcb_list->count--;
            return 0; // Success
        }
        previous=current;
        current=current->next;
    }
    return -1;
}



// Initialize queue
void initializePriorityQueue(PcbPriorityQueue* queue) {
    if (!queue) return;
    queue->front = NULL;
    queue->rear = NULL;
}

// Check if queue is empty
bool isPriorityQueueEmpty(PcbPriorityQueue* queue) {
    return (!queue || queue->front == NULL);
}

// Enqueue PCB based on priority (lower number = higher priority)
bool enqueuePriority(PcbPriorityQueue* queue, PCB* pcb) {
    if (!queue || !pcb) return false;

    PcbNode* node = malloc(sizeof(PcbNode));
    if (!node) return false;
    node->pcb = pcb;
    node->next = NULL;

    if (isPriorityQueueEmpty(queue)) {
        queue->front = node;
        queue->rear = node;
        return true;
    }

    PcbNode* current = queue->front;
    PcbNode* prev = NULL;

    while (current && current->pcb->priority <= pcb->priority) {
        prev = current;
        current = current->next;
    }

    if (!prev) { // insert at front
        node->next = queue->front;
        queue->front = node;
    } else { // middle or end
        prev->next = node;
        node->next = current;
        if (!current) queue->rear = node;
    }

    return true;
}

// Dequeue PCB (highest priority, front of queue)
PCB* dequeuePriority(PcbPriorityQueue* queue) {
    if (isPriorityQueueEmpty(queue)) return NULL;

    PcbNode* temp = queue->front;
    PCB* pcb = temp->pcb;
    queue->front = temp->next;
    if (!queue->front) queue->rear = NULL;
    free(temp);
    return pcb;
}

// Peek at front PCB
PCB* peekPriorityFront(PcbPriorityQueue* queue) {
    if (isPriorityQueueEmpty(queue)) return NULL;
    return queue->front->pcb;
}

// Free entire queue
void freePriorityQueue(PcbPriorityQueue* queue) {
    if (!queue) return;
    PcbNode* current = queue->front;
    PcbNode* next;
    while (current) {
        next = current->next;
        free(current);
        current = next;
    }
    queue->front = NULL;
    queue->rear = NULL;
}

bool removeFromQueue(PcbPriorityQueue* queue, PCB* pcb) {
    if (!queue || !pcb || isPriorityQueueEmpty(queue))
        return false;

    PcbNode* current = queue->front;
    PcbNode* prev = NULL;

    while (current) {
        if (current->pcb == pcb) {
            // Found node to remove
            if (!prev) {
                // Removing front
                queue->front = current->next;
                if (!queue->front) queue->rear = NULL;
            } else {
                prev->next = current->next;
                if (!current->next) queue->rear = prev;
            }
            free(current);
            return true;
        }
        prev = current;
        current = current->next;
    }

    // PCB not found in queue
    return false;
}

bool updatePriority(PcbPriorityQueue* queue, PCB* pcb, int newPriority) {
    if (!queue || !pcb)
        return false;

    // Change the priority in the PCB itself
    pcb->priority = newPriority;

    // Remove from queue if it exists
    removeFromQueue(queue, pcb);

    // Re-insert based on new priority
    return enqueuePriority(queue, pcb);
}





int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}



int get_count_PCB(PCB_linked_list* pcb_list){
    return pcb_list->count;
}
PCB* get_PCB_entry(PCB_linked_list* pcb_list, int process_id){
    if(pcb_list->head==NULL){
        return NULL; // Empty list
    }

    PCB_node* current=pcb_list->head;

    while(current!=NULL){
        if(current->PCB_entry.process_id==process_id){
            return &current->PCB_entry;
        }
        current=current->next;
    }
    return NULL; // Not found
}
