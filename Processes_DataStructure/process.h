#ifndef PROCESS_H
#define PROCESS_H
#define true 1
#define false 0
typedef short bool;

// Note: request structure moved to memory_structures.h
#include "../memory.h"

typedef struct process{
    int ID;  
    int ARRIVAL_TIME;
    int PRIORITY;
    int RUNNING_TIME;
    int DEPENDENCY_ID;
    int disk_base;  
    int time;
    int limit;      
    bool first_time;
    request memory_requests[100];  
    int num_requests;          
    int current_request;       
} process;

#endif