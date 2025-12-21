#include "headers.h"
#include <signal.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include "Processes_DataStructure/process.h"

void clearResources(int);

typedef char* string;

int selected_Algorithm_NUM=-1;
string selected_Algorithm=NULL;
int time_quantum;
int MESSAGE_ID;

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    signal(SIGKILL, clearResources);

    process process_list[max];
    int count=0;
    
    FILE*input_File=fopen(argv[1],"r");
    if(input_File==NULL)
    {
        perror("\nError in openning file!\n");
        exit(1);
    }
    
    char line[2*max];
    char*s=fgets(line,2*max,input_File);
    while(s!=NULL)
    {
        if(line[0]=='#'){
            s=fgets(line,2*max,input_File);
            continue;
        }
        sscanf(line,"%d %d %d %d %d %d %d",
               &process_list[count].ID,
               &process_list[count].ARRIVAL_TIME,
               &process_list[count].RUNNING_TIME,
               &process_list[count].PRIORITY,
               &process_list[count].DEPENDENCY_ID,
               &process_list[count].disk_base,
               &process_list[count].limit);
        count++;
        s=fgets(line,2*max,input_File);
    }
    fclose(input_File);

    // Read memory request files for each process
    for(int i = 0; i < count; i++) {
        char filename[50];
        sprintf(filename, "requests_%d.txt", process_list[i].ID);
        
        FILE* request_file = fopen(filename, "r");
        if(request_file == NULL) {
            printf("Warning: Could not open request file %s for process %d\n", 
                   filename, process_list[i].ID);
            process_list[i].num_requests = 0;
            process_list[i].current_request = 0;
            continue;
        }
        
        char req_line[200];
        // Skip header line
        fgets(req_line, sizeof(req_line), request_file);
        
        process_list[i].num_requests = 0;
        
        while(fgets(req_line, sizeof(req_line), request_file) != NULL) {
            if(req_line[0] == '#' || req_line[0] == '\n' || req_line[0] == '\r') {
                continue;
            }
            
            int time;
            char binary_addr[20];
            char rw;
            
            // Parse line
            if(sscanf(req_line, "%d %s %c", &time, binary_addr, &rw) != 3) {
                continue;
            }
            
            // Convert 10-bit binary string to integer
            int address = 0;
            for(int bit = 0; bit < 10 && binary_addr[bit] != '\0'; bit++) {
                address <<= 1;
                if(binary_addr[bit] == '1') {
                    address |= 1;
                }
            }
            
            // Store request
            int idx = process_list[i].num_requests;
            if(idx < 100) {
                process_list[i].memory_requests[idx].time = time;
                process_list[i].memory_requests[idx].address = address;
                process_list[i].memory_requests[idx].rw = rw;
                process_list[i].num_requests++;
            }
        }
        
        fclose(request_file);
    }
    
    printf("1.HPF\n2.SRTN\n3.RR\n");
    selected_Algorithm = malloc(10 * sizeof(char));
    while(selected_Algorithm_NUM==-1)
    {
        printf("Enter The No. of Scheduling Technique : ");
        scanf("%d", &selected_Algorithm_NUM);
        switch (selected_Algorithm_NUM)
        {
            case 1:
            strcpy(selected_Algorithm,"HPF");
            break;
            case 2:
            strcpy(selected_Algorithm,"SRTN");
            break;
            case 3:
            strcpy(selected_Algorithm,"RR");
            printf("Enter Time Quantum : ");
            scanf("%d", &time_quantum);
            break;
        default:
        selected_Algorithm_NUM=-1;
        printf("Invalid Algorithm\n");
            break;
        }
    }
    
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
        execl("./scheduler.out", "scheduler.out", 
              selected_Algorithm_NUM_str, time_quantum_str, total_process, NULL);
        perror("Error in executing scheduler process");
        exit(0);
    }
    
    key_t key_msg_process = ftok("keyfile", 'A');
    MESSAGE_ID = msgget(key_msg_process, 0666|IPC_CREAT);
    if(MESSAGE_ID==-1){
        printf("Error In Creating Message Queue!\n");
        exit(1);
    }
    
    message_buf PROCESS_MESSAGE;
    initClk();
    int c = 0;
    int x = getClk();
    
    for(int i=0; i<count; i++){
        while(c <= i){
            x = getClk();
            if(x >= process_list[i].ARRIVAL_TIME){
                process_list[i].first_time = true;
                PROCESS_MESSAGE.msgtype = 2;
                PROCESS_MESSAGE.p = process_list[i];
                if(msgsnd(MESSAGE_ID, &PROCESS_MESSAGE, sizeof(process), !IPC_NOWAIT) == -1){
                    printf("Error In Sending Message To Scheduler!\n");
                } else {
                    c++;
                    break;
                }
            }
        }
    }
    
    // Wait for child processes
    wait(NULL);
    wait(NULL);
    
    destroyClk(true);
    return 0;
}

void clearResources(int signum)
{
    msgctl(MESSAGE_ID, IPC_RMID, NULL);
    destroyClk(true);
    exit(0);
}