#include "Processes_DataStructure/process.h"
#include "Processes_DataStructure/process_priority_queue.h"
#include "Processes_DataStructure/process_queue.h"
#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*---------------------------------Omar Syed------------------------------------*/

int MESSAGE_ID;

/*---------------------------------QUEUES&PCB------------------------------------*/

//cpu bound --> no blocking queue
PCB PCB_ARRAY[max]; 
process_queue READY_QUEUE;
process_priority_queue READY_PRIORITY_QUEUE;

int process_count=0;
int RUNNING_PROCESS_PID=-1;
int TIME =0;
void CREATE_PROCESS_PCB(process p){
    PCB_ARRAY[process_count].p=p;
    PCB_ARRAY[process_count].REMAINING_TIME=p.RUNNING_TIME;
    PCB_ARRAY[process_count].WAITING_TIME=0;
    PCB_ARRAY[process_count].RUNNING_TIME=0;
    PCB_ARRAY[process_count].START_TIME=-1;
    PCB_ARRAY[process_count].LAST_EXECUTED_TIME=-1;
    PCB_ARRAY[process_count].FINISH_TIME=-1;
    PCB_ARRAY[process_count].is_completed=false;
    PCB_ARRAY[process_count].process_state=READY;
    process_count++;
}


/*---------------------------------Omar Syed------------------------------------*/
int main(int argc, char * argv[])
{
    initClk();
    
    //TODO implement the scheduler :)
    /*---------------------------Omar Syed------------------------------------*/
    initialize_queue(&READY_QUEUE);
    initialize_priority_queue(&READY_PRIORITY_QUEUE);
    int selected_Algorithm_NUM=atoi(argv[1]);
    int TIME_QUANTUM=atoi(argv[2]);// if RR 3
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    message_buf PROCESS_MESSAGE;
    int process_count=0;
    /*---------------------------Omar Syed------------------------------------*/
while(1){
if(msgrcv(MESSAGE_ID,&PROCESS_MESSAGE, sizeof(message_buf),2,IPC_NOWAIT)!=-1){
    CREATE_PROCESS_PCB(PROCESS_MESSAGE.p);
    if(selected_Algorithm_NUM==3){//HPF
        enqueue(&READY_QUEUE,PROCESS_MESSAGE.p);
}
else{
    enqueue_priority(&READY_PRIORITY_QUEUE,PROCESS_MESSAGE.p);
}
int RUNNING_PROCESS_PID=fork();
if(RUNNING_PROCESS_PID==0){
    char remaining_time_str[100];
    sprintf(remaining_time_str, "%d", PROCESS_MESSAGE.p.RUNNING_TIME);
    execl("./process.out","process.out",remaining_time_str,NULL);
    perror("Error In Executing Process!\n");
    exit(1);    
}

/*---------------------------Omar Syed------------------------------------*/
    //upon termination release the clock resources.
    
    destroyClk(true);
}
}
}