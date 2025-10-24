#include "headers.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define max 100

void clearResources(int);

/*---------------------------Omar Syed------------------------------------*/


typedef char* string;

int schselected_Algorithm_NUM=-1;
string selected_Algorithm;
int time_quantum;

/*---------------------------Omar Syed------------------------------------*/

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources); // TODO: what does this do @omarelsayed
    process process_list[max];
    int count=0;
    // TODO Initialization
    // 1. Read the input files.

    /*---------------------------Omar Syed------------------------------------*/
    FILE*input_File=fopen(argv[1],"r");//open file
    if(input_File==NULL)
    {
        perror("\nError in openning file!\n");
    }
    char line[2*max];
    char*s=fgets(line,2*max,input_File);//read first line
        while(s!=NULL)
    {
            if(line[0]=='#'){//check #
                s=fgets(line,2*max,input_File);
                continue;
            }
        sscanf(line,"%d %d %d %d %d",&process_list[count].ID,&process_list[count].ARRIVAL_TIME,&process_list[count].RETURN_CODE,&process_list[count].PRIORITY,&process_list[count].DEPENDENCY_ID);//read int from str
         count++;
        s=fgets(line,2*max,input_File);
    }
        fclose(input_File);
        
        
        // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
        
        /*---------------------------Omar Syed------------------------------------*/
        printf("1.HPF\n2.SRTN\n3.RR\n");
        while(schselected_Algorithm_NUM==-1)
    {
        printf("Enter The No. of Scheduling Technique : ");
        scanf("%d", &schselected_Algorithm_NUM);
        switch (schselected_Algorithm_NUM)
        {
            case 1:
            selected_Algorithm="HPF"; // highest priority first
            break;
            case 2:
            selected_Algorithm="SRTN"; // shortest remaining time next
            break;
            case 3:
            selected_Algorithm="RR"; // round robin
            printf("Enter Time Quantum : ");
            scanf("%d", &time_quantum);
            break;
        default:
        schselected_Algorithm_NUM=-1;
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
        execl("./clk.out","",NULL);
        perror("clk failed to execute\n");
    }
    else{ //parent
        int pid =fork();//child scheduler
        if(pid==0){
            //child scheduler
            execl("./scheduler.out","",NULL);
            perror("scheduler failed to execute\n");
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
