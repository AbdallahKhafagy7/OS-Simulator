#include "headers.h"

int * shmaddr;

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
    pcb->STARTED=false;
    pcb->WAITING_TIME=0;
    pcb->blocked_time=0;
    pcb->num_requests=0;
    pcb->num_pages=0;
    pcb->disk_base=0;
    pcb->limit=0;
    pcb->execution_time=0;
    
    // Initialize page table pointers
    pcb->page_table.entries = NULL;
    pcb->page_table.num_pages = 0;
    pcb->page_table.physical_page_number = -1;
    pcb->page_table.disk_base = 0;
}

void INITIALIZE_PCB_Node(PCB_node* pcb_node){
    INITIALIZE_PCB(&pcb_node->PCB_entry);
    pcb_node->next=NULL;
}

void INITIALIZE_PCB_Linked_List(PCB_linked_list* pcb_list){
    pcb_list->head = NULL;
    pcb_list->tail = NULL;
    pcb_list->count = 0;
}

void ADD_PCB(PCB_linked_list* pcb_list, PCB pcb_entry){
    PCB_node* new_node=(PCB_node*)malloc(sizeof(PCB_node));
    if (!new_node){
        perror("Memory allocation failed for PCB_node\n");
        return;
    }

    INITIALIZE_PCB_Node(new_node);
    new_node->PCB_entry=pcb_entry;
    new_node->next=NULL;

    if(pcb_list->head==NULL){
        pcb_list->head=new_node;
        pcb_list->tail=new_node;
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
            if(previous==NULL){
                pcb_list->head=current->next;
                if(pcb_list->head==NULL){
                    pcb_list->tail=NULL;
                }
            }else{
                previous->next=current->next;
                if(current==pcb_list->tail){
                    pcb_list->tail=previous;
                    pcb_list->tail->next=NULL;
                }
            }
            free(current);
            pcb_list->count--;
            return 0;
        }
        previous=current;
        current=current->next;
    }
    return -1;
}

void initializePriorityQueue(PcbPriorityQueue* queue) {
    if (!queue) return;
    queue->front = NULL;
    queue->rear = NULL;
}

bool isPriorityQueueEmpty(PcbPriorityQueue* queue) {
    return (!queue || queue->front == NULL);
}

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

    if (!prev) {
        node->next = queue->front;
        queue->front = node;
    } else {
        prev->next = node;
        node->next = current;
        if (!current) queue->rear = node;
    }

    return true;
}

PCB* dequeuePriority(PcbPriorityQueue* queue) {
    if (isPriorityQueueEmpty(queue)) return NULL;

    PcbNode* temp = queue->front;
    PCB* pcb = temp->pcb;
    queue->front = temp->next;
    if (!queue->front) queue->rear = NULL;
    free(temp);
    return pcb;
}

PCB* peekPriorityFront(PcbPriorityQueue* queue) {
    if (isPriorityQueueEmpty(queue)) return NULL;
    return queue->front->pcb;
}

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
            if (!prev) {
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

    return false;
}

bool updatePriority(PcbPriorityQueue* queue, PCB* pcb, int newPriority) {
    if (!queue || !pcb)
        return false;

    pcb->priority = newPriority;
    removeFromQueue(queue, pcb);
    return enqueuePriority(queue, pcb);
}

int getClk() {
    return *shmaddr;
}

void initClk() {
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1) {
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}

void destroyClk(bool terminateAll) {
    shmdt(shmaddr);
    if (terminateAll) {
        killpg(getpgrp(), SIGINT);
    }
}

int get_count_PCB(PCB_linked_list* pcb_list){
    return pcb_list->count;
}

PCB* get_PCB_entry(PCB_linked_list* pcb_list, int process_id){
    if(pcb_list->head==NULL){
        return NULL;
    }

    PCB_node* current=pcb_list->head;

    while(current!=NULL){
        if(current->PCB_entry.process_id==process_id){
            return &current->PCB_entry;
        }
        current=current->next;
    }
    return NULL;
}

PCB* get_pcb(PCB* pcb_array, int process_count, int process_id){
    for(int i = 0; i < process_count; i++){
        if(pcb_array[i].process_id == process_id){
            return &pcb_array[i];
        }
    }
    return NULL;
}

int get_pcb_index(PCB* pcb_array, int process_count, int process_id){
    for(int i = 0; i < process_count; i++){
        if(pcb_array[i].process_id == process_id){
            return i;
        }
    }
    return -1;
}

void remove_pcb(PCB* pcb_array, int *process_count, int process_id){
    int k = get_pcb_index(pcb_array, *process_count, process_id);
    if(k == -1) return;
    
    for(int i = k; i < *process_count - 1; i++){
        pcb_array[i] = pcb_array[i + 1];
    }
    (*process_count)--;
}