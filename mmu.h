#ifndef MMU_H
#define MMU_H

typedef short bool;

#define ADDRESS_BITS 10       // 10-bit address space
#define PAGE_SIZE 16          // 16 bytes per page
#define RAM_SIZE 512          // 512 bytes RAM
#define NUM_PHYSICAL_PAGES 32 // 32 page frames
#define DISK_ACCESS_TIME 10   // 10 cycles for disk access
#define OFFSET_BITS 4         // 4 bits for offset
#define VPN_BITS 6            // 6 bits for virtual page number
#define PPN_BITS 5            // 5 bits for physical page number
#define MAX_VIRTUAL_PAGES 64  // Maximum virtual pages per process
#define MAX_PHYSICAL_PAGES 32 // Maximum physical pages

typedef struct PCB_struct PCB;

typedef struct
{
    bool present;             // Page present in memory
    bool modified;            // Dirty bit
    bool referenced;          // Reference bit
    int physical_page_number; // Physical page number
} PageTableEntry;

typedef struct
{
    bool is_free;            // Is this frame free?
    int process_id;          // process ID
    int virtual_page_number; // Virtual page number mapped here
    bool referenced;         // Reference bit
    bool modified;           // Dirty bit
    bool locked;             // Locked for I/O (cannot be evicted)
    bool is_page_table;
} PhysicalPage;

typedef struct
{
    PageTableEntry *entries;  // Array of page table entries
    int num_pages;            // Number of virtual pages needed
    int physical_page_number; // Physical page holding page table
    int disk_base;            // Base page number on disk
} ProcessPageTable;

typedef struct FreePageNode
{
    int page_number;
    struct FreePageNode *next;
} FreePageNode;

typedef struct
{
    PhysicalPage pages[NUM_PHYSICAL_PAGES];
    int free_page_count;
    int clock_pointer;            // For Second Chance algorithm
    FreePageNode *free_list_head; // Head of free page list
    int page_faults;
    int disk_reads;
    int disk_writes;
    int page_replacements;
} MemoryManager;

// Global memory manager instance
extern MemoryManager mem_mgr;

// Function declarations
void add_to_free_list(int page_number);
void print_free_list();
void init_free_list();
int remove_from_free_list();
int Request(PCB *pcb, int process_count, int process_id, int virtual_page, char readwrite_flag, int current_time);
void init_memory(void);
int allocate_free_page(int process_id, int virtual_page);
void free_process_pages(int process_id, PCB *pcb);
int second_chance_replacement();
int translate_address(int process_id, int virtual_address, PCB *pcb, char rw_flag);
void load_page_from_disk(int process_id, int virtual_page, int physical_page, int disk_base);
void swap_page_to_disk(int process_id, int virtual_page, int physical_page);
void print_memory_log(const char *format, ...);
void close_memory_log(void);
void print_memory_status(void);
int init_process_page_table(PCB *pcb);
int allocate_process_page_table(PCB *pcb);

// Helper functions to access memory manager statistics
int get_free_page_count(void);
int get_page_faults(void);
int get_page_replacements(void);
int get_disk_reads(void);
int get_disk_writes(void);

// Inline functions
static inline int get_vpn(int virtual_address)
{
    return virtual_address >> OFFSET_BITS;
}

static inline int get_offset(int virtual_address)
{
    return virtual_address & (PAGE_SIZE - 1);
}

static inline int get_physical_address(int physical_page, int offset)
{
    return (physical_page << OFFSET_BITS) | offset;
}

#endif // MMU_H