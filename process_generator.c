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
        sscanf(line,"%d %d %d %d %d %d %d",&process_list[count].ID,&process_list[count].ARRIVAL_TIME,&process_list[count].RUNNING_TIME,&process_list[count].PRIORITY,&process_list[count].DEPENDENCY_ID,&process_list[count].disk_base,&process_list[count].limit);//read int from str
         count++;
        s=fgets(line,2*max,input_File);
    }
        fclose(input_File);


        
   for(int i = 0; i < count; i++) {
    char filename[50];
    sprintf(filename, "requests_%d.txt", i+1);
    
    FILE* request_file = fopen(filename, "r");
    if(request_file == NULL) {
        printf("Warning: Could not open request file %s for process %d\n", filename, i+1);
        process_list[i].num_requests = 0;
        process_list[i].current_request = 0;
        continue;
    }

    //printf("Reading requests from %s for process %d\n", filename, i+1);
    
    char line[200];
    
   
    fgets(line, sizeof(line), request_file);
    
   
    process_list[i].num_requests = 0;
    process_list[i].current_request = 0;
    
    while(fgets(line, sizeof(line), request_file) != NULL) {
        if(line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        int time;
        char binary_addr[20];
        char rw;
        char* token;
        char* rest = line;

        token = strtok(rest, " \t\n");
        if(token == NULL) continue;
        time = atoi(token);
        
        token = strtok(NULL, " \t\n");
        if(token == NULL) continue;
        strncpy(binary_addr, token, sizeof(binary_addr)-1);
        binary_addr[sizeof(binary_addr)-1] = '\0';
        
        token = strtok(NULL, " \t\n");
        if(token == NULL) continue;
        rw = token[0];
        
        int address = 0;
        for(int bit = 0; bit < 10 && binary_addr[bit] != '\0'; bit++) {
            if(binary_addr[bit] == '1') {
                address = (address << 1) | 1;
            } else if(binary_addr[bit] == '0') {
                address = address << 1;
            }
        }

            int idx = process_list[i].num_requests;
            process_list[i].memory_requests[idx].time = time;
            process_list[i].memory_requests[idx].address = address;
            process_list[i].memory_requests[idx].rw = rw;
            process_list[i].num_requests++;
            
            // printf("  Request %d: time=%d, address=%d (%s), r/w=%c\n", 
            //        idx+1, time, address, binary_addr, rw);
    }
    
    fclose(request_file);
 //   printf("  Total requests loaded: %d\n\n", process_list[i].num_requests);
}

        // for (int i=0; i < count; i++)
        // {
        //     printf("process id is %d\n", process_list[i].ID);
        //     printf("process arrival time is %d\n", process_list[i].ARRIVAL_TIME);
        //     printf("process running time is %d\n", process_list[i].RUNNING_TIME);
        //     printf("process priority is %d\n", process_list[i].PRIORITY);
        //     printf("process dependency id is %d\n", process_list[i].DEPENDENCY_ID);
        //     printf("process disk base is %d\n", process_list[i].disk_base); 
        //     printf("process limit is %d\n", process_list[i].limit);
        //     printf("process num_requests is %d\n", process_list[i].num_requests);
        //     for(int j=0;j<process_list[i].num_requests;j++){
        //         printf("    request %d : time %d , address %d , rw %c\n",j,process_list[i].memory_requests[j].time,process_list[i].memory_requests[j].address,process_list[i].memory_requests[j].rw);
        // }
        // }
        
        
        
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
        exit(0);
    }

    int scheduler_pid = fork();
    if (scheduler_pid == 0)
    {
        char selected_Algorithm_NUM_str[100];
        char time_quantum_str[100];
        char total_process[20];
        sprintf(total_process, "%d", count);
        sprintf(selected_Algorithm_NUM_str, "%d", selected_Algorithm_NUM);
        sprintf(time_quantum_str, "%d", time_quantum);
        execl("./scheduler.out", "scheduler.out", selected_Algorithm_NUM_str, time_quantum_str,total_process, NULL);
        perror("Error in executing scheduler process");
        exit(0);
    }
    
    
    
    
    
    /*---------------------------Omar Syed------------------------------------*/
    /*---------------------------Omar Syed------------------------------------*/

    // 4. Use this function after creating the clock process to initialize clock
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
    }
    //printf(" message queue ID is %d \n",MESSAGE_ID);
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
                if(msgsnd(MESSAGE_ID,&PROCESS_MESSAGE,sizeof(process),!IPC_NOWAIT)==-1){
                    printf("Error In Sending Message To Scheduler!\n");
                }else{
                    //printf("Process with id %d sent to scheduler at time %d\n",process_list[i].limit,getClk());
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
        exit(0);
}
