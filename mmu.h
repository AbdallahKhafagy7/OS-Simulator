#ifndef MMU_H
#define MMU_H

struct PCB_struct;

typedef short bool;
// System Constants
#define ADDRESS_BITS 10          //10-bit address 
#define PAGE_SIZE 16             // page size 16-bit
#define RAM_SIZE 512             // RAM size 512-bit --> 32 pages
#define NUM_PHYSICAL_PAGES 32    // 32 page frames
#define DISK_ACCESS_TIME 10      // 10 cycles for disk access
#define OFFSET_BITS 4            // 4 bit offset
#define VPN_BITS 6               //  6 bits for virtual page number
#define PPN_BITS 5               //  5 bits for physical page number  

//PTE Structure
typedef struct {
    bool present;               // present bit
    bool modified;              // dirty bit
    bool referenced;            // reefernce bit
    int physical_page_number;   // Physical page number  0 --> 31
} PageTableEntry;

// frame structure
typedef struct {
    bool is_free;            // has process assigned or not
    int process_id;          // process id or pid for swapping to control stop and continue
    int virtual_page_number; // Virtual page number for process existing in this frame
    bool referenced;         // reference bit
    bool modified;           // Dirty bit
} PhysicalPage;

// Process Page Table  for each process
typedef struct {
    PageTableEntry *entries; // Array of page table entries 0 to 63
    int num_pages;           // Number of pages process needs
    int physical_page_number;       // Physical page holding the page table -1 if not loaded   PPN
    int disk_base;           // Base address on disk for this process
} ProcessPageTable;

// Global Memory Management Structure
typedef struct {
    PhysicalPage pages[NUM_PHYSICAL_PAGES];  // Physical memory frames
    int free_page_count;                      // Count of free pages
    int clock_pointer;                           // For Clock algorithm
} MemoryManager;

typedef struct {
    int free;       // 1 = free, 0 = used
    int pid;        // owning process
    int vpn;        // virtual page number
    int ref;        // for Second Chance
    int dirty;      // modified page
    int isTable;    // 1 if this frame holds a page table
} Frame;





void init_memory();
int allocate_free_page(int process_id, int virtual_page);
int second_chance_replacement();
void handle_page_fault(struct PCB_struct *pcb, int process_Count ,int process_id, int virtual_page, char readwrite_flag);
void load_page_from_disk(int process_id, int virtual_page, int physical_page);
void swap_page_to_disk(int process_id, int virtual_page, int physical_page);
int translate_address(int process_id, int virtual_address);
void free_process_pages(int process_id);
void print_memory_log(const char* format, ...);
void signal_page_loaded(int process_id, int virtual_page);


#endif 