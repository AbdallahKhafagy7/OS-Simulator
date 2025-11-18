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
<<<<<<< HEAD
#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_queue.h"
#include "Processes_DataStructure/process_priority_queue.h"

=======
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "Processes_DataStructure/process.h"

#define max 100
>>>>>>> 3a83696511a7d3b8add38931a3efd33f92be1480
typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
<<<<<<< HEAD
=======
// struct 

typedef enum { Ready, Running, Finished} state;

typedef char* string;

struct message_buf{
    long msgtype;
    process p;
}typedef message_buf ;

struct PCB_struct{
    process p;
    state process_state;
    int REMAINING_TIME;
    int WAITING_TIME;
    int RUNNING_TIME;
    int START_TIME;
    int LAST_EXECUTED_TIME;
    int FINISH_TIME;
    bool is_completed;
    struct PCB_struct* next;
}typedef PCB;

void INITIALIZE_PCB(PCB* pcb){
    pcb->process_state=Ready;
    pcb->REMAINING_TIME=0;
    pcb->WAITING_TIME=0;
    pcb->RUNNING_TIME=0;
    pcb->START_TIME=-1;
    pcb->LAST_EXECUTED_TIME=-1;
    pcb->FINISH_TIME=-1;
    pcb->is_completed=false;
    pcb->next=NULL;
}

void enqueue_PCB(PCB** head,PCB* new_PCB){
    if(*head==NULL){
        *head=new_PCB;
        return;
    }
    PCB* temp=*head;
    while(temp->next!=NULL){
        temp=temp->next;
    }
    temp->next=new_PCB;
}

void print_PCB_list(PCB* head){
    PCB* temp=head;
    while(temp!=NULL){
        if (temp->process_state==Ready)
        {
            printf("Process ID: %d, State: Ready , Remaining Time: %d\n",temp->p.ID,temp->REMAINING_TIME);
            
        }
        else if (temp->process_state==Running)
        {
            printf("Process ID: %d, State: Running , Remaining Time: %d\n",temp->p.ID,temp->REMAINING_TIME);
            
        }
        else if (temp->process_state==Finished)
        {
            printf("Process ID: %d, State: Finished , Remaining Time: %d\n",temp->p.ID,temp->REMAINING_TIME);
            
        }
        temp=temp->next;
    }
}

void clear_PCB_list(PCB* head){
    PCB* temp=head;
    while(temp!=NULL){
        PCB* to_free=temp;
        temp=temp->next;
        free(to_free);
    }
}

void dequeue_PCB(PCB** head){
    if(*head==NULL){
        return;
    }
    PCB* temp=*head;
    *head=(*head)->next;
    temp=NULL;
}

>>>>>>> 3a83696511a7d3b8add38931a3efd33f92be1480


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
