#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include "Processes_DataStructure/process.h"


void clearResources(int);
 

/*---------------------------Omar Syed------------------------------------*/
/*
1-Read From File
2-Create Message Buffer
3-fork Scheduler & Clk
4-send processes to the scheduler
5-clear resource 
Done by Omar Syed
*/

typedef char* string;



int selected_Algorithm_NUM=-1;
string selected_Algorithm=NULL;
int time_quantum;
int MESSAGE_ID;


/*---------------------------Omar Syed------------------------------------*/

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources); // TODO: what does this do @omarelsayed
    signal(SIGKILL, clearResources); // TODO: what does this do @omarelsayed

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
        sscanf(line,"%d %d %d %d %d",&process_list[count].ID,&process_list[count].ARRIVAL_TIME,&process_list[count].RUNNING_TIME,&process_list[count].PRIORITY,&process_list[count].DEPENDENCY_ID);//read int from str
         count++;
        s=fgets(line,2*max,input_File);
    }
        fclose(input_File);
        
        
        // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
        
        /*---------------------------Omar Syed------------------------------------*/
        printf("1.HPF\n2.SRTN\n3.RR\n");
        if (selected_Algorithm == NULL) selected_Algorithm = malloc(10 * sizeof(char));
        while(selected_Algorithm_NUM==-1)
    {
        printf("Enter The No. of Scheduling Technique : ");
        scanf("%d", &selected_Algorithm_NUM);
        switch (selected_Algorithm_NUM)
        {
            case 1:
            strcpy(selected_Algorithm,"HPF"); // highest priority first
            break;
            case 2:
            strcpy(selected_Algorithm,"SRTN"); // shortest remaining time next
            break;
            case 3:
            strcpy(selected_Algorithm,"RR"); // round robin
            printf("Enter Time Quantum : ");
            scanf("%d", &time_quantum);
            break;
        default:
        selected_Algorithm_NUM=-1;
        printf("Invalid Algorithm\n");
            break;
        }
    }
    /*---------------------------Omar Syed------------------------------------*/

    // 3. Initiate and create the scheduler and clock processes.

    
    int clk_pid = fork();
    if (clk_pid == 0)
    {
        execl("./clk.out","clk.out",NULL);
        perror("Error in executing clock process");
        exit(1);
    }

    int scheduler_pid = fork();
    if (scheduler_pid == 0)
    {
        char selected_Algorithm_NUM_str[100];
        char time_quantum_str[100];
        sprintf(selected_Algorithm_NUM_str, "%d", selected_Algorithm_NUM);
        sprintf(time_quantum_str, "%d", time_quantum);
        execl("./scheduler.out", "scheduler.out", selected_Algorithm_NUM_str, time_quantum_str, NULL);
        perror("Error in executing scheduler process");
        exit(1);
    }
    
    
    
    
    
    /*---------------------------Omar Syed------------------------------------*/
    /*---------------------------Omar Syed------------------------------------*/

    // 4. Use this function after creating the clock process to initialize clock
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    printf(" message queue ID is %d \n",MESSAGE_ID);
    message_buf PROCESS_MESSAGE;
    initClk();
    int c = 0;
    int x = getClk();
    printf("current time is %d\n", x);
    for(int i=0;i<count;i++){
        while(c<=i){
            x=getClk();
            if(x==process_list[i].ARRIVAL_TIME){
                process_list[i].first_time=true;
                PROCESS_MESSAGE.msgtype=2;
                PROCESS_MESSAGE.p=process_list[i];
                if(msgsnd(MESSAGE_ID,&PROCESS_MESSAGE,sizeof(message_buf),!IPC_NOWAIT)==-1){
                    printf("Error In Sending Message To Scheduler!\n");
                }else{
                    printf("Process with id %d sent to scheduler at time %d\n",process_list[i].ID,getClk());
                    c++;
                    break;
                }
            }
        }
    }
    // TODO Generation Main Loop
    
    // 5. Create a data structure for processes and provide it with its parameters.
    
    // 6. Send the information to the scheduler at the appropriate time.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    //
    // raise(SIGINT);
    for (int i=0; i < 2; i++)
    {
        wait(NULL);
    }
    destroyClk(true);
}
void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    msgctl(MESSAGE_ID, IPC_RMID, NULL); 
        destroyClk(true);
        exit(1);
}
