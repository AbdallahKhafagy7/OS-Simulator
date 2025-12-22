#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define null 0
#define PAGE_SIZE 16  // 16 bytes per page

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

void initializeProcessData(struct processData *processes, int* lastArrival, int i, int *nextDiskPage) {
    processes[i].id = i + 1;
    processes[i].arrivaltime = *lastArrival + rand() % 11;
    *lastArrival = processes[i].arrivaltime;
    processes[i].runningtime = 1 + rand() % 30;
    processes[i].priority = rand() % 11;
    processes[i].dependencyId = -1;
    processes[i].base = *nextDiskPage;
    
    // Generate limit as NUMBER OF PAGES
    // Project spec shows numbers like 1000, 2000
    // These are virtual pages, can be more than physical memory (32 pages)
    processes[i].limit = 100 + (rand() % 10) * 100;  // 100-1000 pages
    
    // Update next disk page
    *nextDiskPage = processes[i].base + processes[i].limit;
}

void assignProcessDependency(struct processData *processes, int i, int numberOfProcesses) {
    int depend = rand() % 2; 

    if (depend && i > 0) {
        int* possible = malloc(numberOfProcesses * sizeof(int));
        if (!possible) return;
        
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
        
        free(possible);
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
    for (int p = 0; p < count; p++) {  
        char filename[64];
        sprintf(filename, "requests_%d.txt", processes[p].id);
        
        FILE *memFile = fopen(filename, "w");
        if (!memFile) {
            printf("Warning: Could not create file %s for process %d\n", filename, processes[p].id);
            continue;
        }
        
        // Write header
        fprintf(memFile, "#%-10s #%-10s #%-10s\n", "time", "addressInBinary", "r/w");
        
        // Get process details
        int runtime = processes[p].runningtime;
        int total_pages = processes[p].limit;
        
        // If runtime is too short for any requests, skip
        if (runtime <= 1) {
            fclose(memFile);
            remove(filename);  // Delete empty file
            continue;
        }
        
        // Generate number of requests (3-10, but within runtime)
        int max_requests = runtime - 1;  // Can't have request at time 0 or at runtime
        if (max_requests <= 0) {
            fclose(memFile);
            remove(filename);
            continue;
        }
        
        int num_requests = 3 + rand() % 8;  // Target 3-10 requests
        if (num_requests > max_requests) {
            num_requests = max_requests;
        }
        
        if (num_requests <= 0) {
            fclose(memFile);
            remove(filename);
            continue;
        }
        
        // Create array for request times
        int *request_times = malloc(num_requests * sizeof(int));
        if (!request_times) {
            fclose(memFile);
            continue;
        }
        
        // Generate unique request times (1 to runtime-1)
        for (int a = 0; a < num_requests; a++) {
            int valid_time = 0;
            while (!valid_time) {
                // Generate time between 1 and runtime-1 inclusive
                request_times[a] = 1 + rand() % (runtime - 1);
                valid_time = 1;
                
                // Check for uniqueness
                for (int b = 0; b < a; b++) {
                    if (request_times[a] == request_times[b]) {
                        valid_time = 0;
                        break;
                    }
                }
            }
        }
        
        // Sort request times
        for (int a = 0; a < num_requests - 1; a++) {
            for (int b = 0; b < num_requests - a - 1; b++) {
                if (request_times[b] > request_times[b + 1]) {
                    int temp = request_times[b];
                    request_times[b] = request_times[b + 1];
                    request_times[b + 1] = temp;
                }
            }
        }
        
        // Generate each request
        for (int a = 0; a < num_requests; a++) {
            int time = request_times[a];
            
            // Generate a valid virtual address within process space
            // Process has total_pages virtual pages, each of PAGE_SIZE bytes
            int max_virtual_page = total_pages - 1;
            int virtual_page;
            
            if (max_virtual_page > 0) {
                virtual_page = rand() % total_pages;
            } else {
                virtual_page = 0;
            }
            
            int offset = rand() % PAGE_SIZE;
            int address = (virtual_page * PAGE_SIZE) + offset;
            
            // Convert to 10-bit binary (for 10-bit address bus)
            // If address is larger than 1023, we need to handle it
            // According to project, virtual addresses can be large
            // But memory system uses 10-bit addresses, so we need to map
            // large virtual addresses to 10-bit representation
            // For simplicity, we'll use the lower 10 bits
            int address_10bit = address & 0x3FF;  // Mask to 10 bits
            
            char binary[11];
            for (int b = 9; b >= 0; b--) {
                binary[9-b] = ((address_10bit >> b) & 1) ? '1' : '0';
            }
            binary[10] = '\0';
            
            char rw = (rand() % 2 == 0) ? 'r' : 'w';
            
            fprintf(memFile, "%-10d\t%-10s\t%-10c\n", time, binary, rw);
        }
        
        free(request_times);
        fclose(memFile);
        
        printf("Created request file for process %d: %d requests\n", 
               processes[p].id, num_requests);
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
    if (argc > 1) {
        no = atoi(argv[1]);
        printf("Generating %d processes (from command line)\n", no);
    } else {
        printf("Please enter the number of processes you want to generate: ");
        if (scanf("%d", &no) != 1 || no <= 0 || no > 1000) {
            printf("Invalid input. Please enter a number between 1 and 1000.\n");
            fclose(pFile);
            return 1;
        }
    }

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
        initializeProcessData(processes, &lastArrival, i, &nextDiskPage);
        assignProcessDependency(processes, i, no);
        writeProcessInFile(pFile, processes, i);
    }
    
    createMemoryAccessFiles(processes, no);
    
    fclose(pFile);
    free(processes);
    
    printf("Successfully generated %d processes in processes.txt\n", no);
    return 0;
}