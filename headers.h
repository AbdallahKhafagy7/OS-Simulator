#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "Processes_DataStructure/process.h"

#define max 400
typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
// struct 

typedef enum { Ready, Running, Finished} state;

typedef char* string;

struct message_buf{
    long msgtype;
    process p;
}typedef message_buf ;

struct PCB_struct{
    int process_id;
    state process_state;
    int REMAINING_TIME;
    int WAITING_TIME;
    int RUNNING_TIME;
    int START_TIME;
    int LAST_EXECUTED_TIME;
    int FINISH_TIME;
    int process_pid;
    int arrival_time;
    bool is_completed;
}typedef PCB;


typedef struct PCB_node
{
    PCB PCB_entry;
    struct PCB_node* next;

}PCB_node;

typedef struct PCB_linked_list
{
    PCB_node* head;
    PCB_node* tail;
    int count;
}PCB_linked_list;


void INITIALIZE_PCB(PCB* pcb){
    pcb->process_state=Ready;
    pcb->REMAINING_TIME=0;
    pcb->WAITING_TIME=0;
    pcb->RUNNING_TIME=0;
    pcb->START_TIME=-1;
    pcb->LAST_EXECUTED_TIME=-1;
    pcb->FINISH_TIME=-1;
    pcb->process_pid=-1;
    pcb->is_completed=false;
}
void INITIALIZE_PCB_Node(PCB_node* pcb_node){
    INITIALIZE_PCB(&pcb_node->PCB_entry);
    pcb_node->next=NULL;
}

void INITIALIZE_PCB_Linked_List(PCB_linked_list* pcb_list){
    pcb_list->head=NULL;
    pcb_list->tail=NULL;
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
        pcb_list->count++;
        return;
    }else{
        pcb_list->tail->next=new_node;
        pcb_list->tail=new_node;
        pcb_list->tail->next=pcb_list->head;
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

// PCB get_PCB_index(PCB_linked_list* pcb_list, int process_id){
//     PCB pcb_entry;
//     pcb_entry.process_id=-1;
//     if(pcb_list->head==NULL){
//         return pcb_entry;
//     }

//     PCB_node* current=pcb_list->head;
//     int index=0;

//     while(current!=NULL){
//         if(current->PCB_entry.process_id==process_id){
//             return current->PCB_entry;
//         }
//         current=current->next;
//         index++;
//     }
//     return pcb_entry; // Not found
// }


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





///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================



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
