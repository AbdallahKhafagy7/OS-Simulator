#include "headers.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#define max 100

void clearResources(int);

/*---------------------------Omar Syed------------------------------------*/

typedef struct process{
    int ID;
    int AT;
    int RT;
    int PR;
    int DID;
} process;

typedef char* string;

int sch=-1;
string sch_algo;
int tq;

/*---------------------------Omar Syed------------------------------------*/

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    process process_list[max];
    int count=0;
    // TODO Initialization
    // 1. Read the input files.

    /*---------------------------Omar Syed------------------------------------*/
    FILE*fp=fopen(argv[1],"r");//open file
    if(fp==NULL){
        perror("\nError in openning file!\n");
    }
    char line[200];
    char*s=fgets(line,200,fp);//read first line
        while(s!=NULL){
            if(line[0]=='#'){//check #
                s=fgets(line,200,fp);
                continue;
            }
        sscanf(line,"%d %d %d %d %d",&process_list[count].ID,&process_list[count].AT,&process_list[count].RT,&process_list[count].PR,&process_list[count].DID);//read int from str
         count++;   
        s=fgets(line,200,fp);
    }
        fclose(fp);
        
        
        // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
        
        /*---------------------------Omar Syed------------------------------------*/
        while(sch==-1){
        printf("1.HPF\n2.SRTN\n3.RR\n");
        printf("Enter The No. of Scheduling Technique : ");
        scanf("%d", &sch);
        switch (sch)
        {
            case 1:
            sch_algo="HPF";
            break;
            case 2:
            sch_algo="SRTN";
            break;
            case 3:
            sch_algo="RR";
            printf("Enter Time Quantum : ");
            scanf("%d", &tq);
            break;
        default:
        sch=-1;
        printf("Invalid Algorithm\n");
            break;
        }
    }
    /*---------------------------Omar Syed------------------------------------*/

    // 3. Initiate and create the scheduler and clock processes.

    /*---------------------------Omar Syed------------------------------------*/

    int pid=fork();//child clk
    if (pid==0)
    {
        //child clk
        execl("./clk","",NULL);
    }
    else{ //parent
        int pid =fork();//child scheduler
        if(pid==0){
            //child scheduler
            execl("./scheduler","",NULL);

        }
    }

    /*---------------------------Omar Syed------------------------------------*/

    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop

    // 5. Create a data structure for processes and provide it with its parameters.

    // 6. Send the information to the scheduler at the appropriate time.

    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
}
