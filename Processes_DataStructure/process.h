#ifndef PROCESS_H
#define PROCESS_H

typedef struct process{
    int ID;  
    int ARRIVAL_TIME;
    int RETURN_CODE;
    int PRIORITY;
    int DEPENDENCY_ID;
} process;

#endif //PROCESS_H