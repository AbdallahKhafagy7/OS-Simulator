#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define null 0

struct processData
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
    int dependencyId;
    int base;
    int limit;
};

void initializeProcessData(struct processData *processes, int*lastArrival,int i,int *nextDiskPage) {
        processes[i].id = i + 1;
        processes[i].arrivaltime = *lastArrival + rand() % 11; // increasing arrival
        *lastArrival = processes[i].arrivaltime;
        processes[i].runningtime = 1 + rand() % 30;
        processes[i].priority = rand() % 11;
        processes[i].dependencyId = -1;
         processes[i].base = *nextDiskPage;
    
    // Generate limit as NUMBER OF PAGES (like 1000, 2000 in PDF example)
    // PDF shows large numbers like 1000, 2000 pages
    // Each process needs many pages (more than fits in RAM)
    processes[i].limit = 100 + (rand() % 10) * 100;  // 100-1000 pages
    
    // Update next disk page
    *nextDiskPage = processes[i].base + processes[i].limit;
    
    // printf("Process %d: disk_page=%d, needs=%d pages, total_bytes=%d\n", 
    //        processes[i].id, 
    //        processes[i].base, 
    //        processes[i].limit,
    //        processes[i].limit * 16);  // Convert to bytes
}

void assignProcessDependency(struct processData *processes, int i,int numberOfProcesses){
    int depend = rand() % 2; 

    if (depend && i > 0) {
        int* possible = malloc(numberOfProcesses * sizeof(struct processData));
        int count = 0;

        for (int j = 0; j < i; j++) {
            int start = processes[j].arrivaltime;
            int end = processes[j].arrivaltime + processes[j].runningtime;

            if (processes[i].arrivaltime >= start && processes[i].arrivaltime <= end) {
                possible[count++] = processes[j].id;
            }
        }

        if (count > 0) {
            int randomIndex = rand() % count;
            processes[i].dependencyId = possible[randomIndex];
        } else {
            processes[i].dependencyId = -1; 
        }
    } else {
        processes[i].dependencyId = -1;
    }
}

void writeProcessInFile(FILE *pFile, struct processData *processes, int i) {
    fprintf(pFile, "%-5d %-10d %-10d %-10d %-20d %-20d %-20d \n",
            processes[i].id,
            processes[i].arrivaltime,
            processes[i].runningtime,
            processes[i].priority,
            processes[i].dependencyId,
            processes[i].base,
            processes[i].limit);
}

void createMemoryAccessFiles(struct processData *processes, int count) {
    for (int p = 0; p < count ; p++) {  
        char filename[50];
        sprintf(filename, "requests_%d.txt", processes[p].id);
        
        FILE *memFile = fopen(filename, "w");
        if (!memFile) continue;
        
        fprintf(memFile, "#%-10s #%-10s #%-10s\n", "time", "addressInBinary", "r/w");
        
        // Generate memory accesses
        // Process has 'limit' pages, each 16 bytes = limit*16 total bytes
        int total_bytes = processes[p].limit * 16;
        
        int num_requests = 3 + rand() % 8;  // 3 to 10 requests
        for (int a = 0; a < num_requests; a++) {
            int time = a + 1;
            
            // Generate address within process space
            int address = rand() % total_bytes;
            if (address >= 1024) address = address % 1024;  // Fit in 10-bit
            
            // Convert to 10-bit binary
            char binary[11];
            for (int b = 9; b >= 0; b--) {
                binary[9-b] = ((address >> b) & 1) ? '1' : '0';
            }
            binary[10] = '\0';
            
            char rw = (rand() % 2 == 0) ? 'r' : 'w';
            
            fprintf(memFile, "%-10d\t%-10s\t%-10c\n", time, binary, rw);
        }
        
        fclose(memFile);
     //   printf("Created: %s\n", filename);
    }
}



int main(int argc, char * argv[])
{
    FILE *pFile;
    pFile = fopen("processes.txt", "w");
    if (!pFile) {
        printf("Error opening file.\n");
        return 1;
    }

    int no;
    printf("Please enter the number of processes you want to generate: ");
    scanf("%d", &no);

    srand(time(null));

    fprintf(pFile, "%-5s %-10s %-10s %-10s %-20s %-20s %-20s\n",
        "#id", "arrival", "runtime", "priority", "dependencyId", "disk_base", "limit");

    struct processData *processes = malloc(no * sizeof(struct processData));
    if (!processes) {
        printf("Memory allocation failed.\n");
        fclose(pFile);
        return 1;
    }

    int lastArrival = 1;
    int nextDiskPage = 0;
    for (int i = 0; i < no; i++) {
        initializeProcessData(processes,&lastArrival,i,&nextDiskPage);

        assignProcessDependency(processes, i,no);
        
        writeProcessInFile(pFile, processes, i);
    }

   // printf("\nTotal disk pages needed: %d pages\n", nextDiskPage);
   // printf("Total disk space: %d bytes\n", nextDiskPage * 16);
   // printf("RAM size: 512 bytes (32 pages)\n");
   // printf("Each process needs more pages than fits in RAM - this is VIRTUAL MEMORY!\n");
    
    createMemoryAccessFiles(processes, no);
    fclose(pFile);
}
