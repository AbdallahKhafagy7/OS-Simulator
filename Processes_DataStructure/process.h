#ifndef PROCESS_H
#define PROCESS_H

typedef struct process{
    int ID;  
    int ARRIVAL_TIME;
<<<<<<< HEAD
    int RETURN_CODE;
=======
    //int RETURN_CODE;
>>>>>>> 3a83696511a7d3b8add38931a3efd33f92be1480
    int PRIORITY;
    int RUNNING_TIME;
    int DEPENDENCY_ID;
    char* NAME;
} process;

#endif //PROCESS_H