#ifndef PROCESS_H
#define PROCESS_H
#define true 1
#define false 0
typedef short bool;

typedef struct process{
    int ID;  
    int ARRIVAL_TIME;
    //int RETURN_CODE;
    int PRIORITY;
    int RUNNING_TIME;
    int DEPENDENCY_ID;
    //char* NAME;
    int first_time;
    
} process;

#endif //PROCESS_H