#ifndef MMU_H
#define MMU_H

typedef short bool;

// System Constants
#define ADDRESS_BITS 10           // 10-bit address space
#define PAGE_SIZE 16              // 16 bytes per page
#define RAM_SIZE 512              // 512 bytes RAM
#define NUM_PHYSICAL_PAGES 32     // 32 page frames (512/16)
#define DISK_ACCESS_TIME 10       // 10 cycles for disk access
#define OFFSET_BITS 4             // 4 bits for offset (log2(16))
#define VPN_BITS 6                // 6 bits for virtual page number (10-4)
#define PPN_BITS 5                // 5 bits for physical page number (log2(32))
#define MAX_VIRTUAL_PAGES 64      // Maximum virtual pages per process (2^6)
#define MAX_PHYSICAL_PAGES 32     // Maximum physical pages (2^5)

// Forward declaration (if PCB is defined elsewhere)
typedef struct PCB_struct PCB;

// Page Table Entry Structure
typedef struct {
    bool present;                // Page present in memory
    bool modified;               // Dirty bit (page has been written)
    bool referenced;             // Reference bit (for Second Chance)
    int physical_page_number;    // Physical page number (0-31)
} PageTableEntry;

// Physical Page Frame Structure
typedef struct {
    bool is_free;                // Is this frame free?
    int process_id;              // Owner process ID
    int virtual_page_number;     // Virtual page number mapped here
    bool referenced;             // Reference bit (for Second Chance)
    bool modified;               // Dirty bit
    bool locked;                 // Locked for I/O (cannot be evicted)
} PhysicalPage;

// Process Page Table Structure
typedef struct {
    PageTableEntry *entries;     // Array of page table entries
    int num_pages;               // Number of virtual pages needed
    int physical_page_number;    // Physical page holding page table (-1 if not loaded)
    int disk_base;               // Base page number on disk
} ProcessPageTable;

// Disk I/O Operation Structure
typedef struct {
    int process_id;
    int virtual_page;
    int physical_page;
    int completion_time;         // When I/O will complete
    char operation;              // 'R' for read, 'W' for write
    bool completed;
} DiskOperation;

typedef struct FreePageNode {
    int page_number;
    struct FreePageNode* next;
} FreePageNode;

// Update MemoryManager structure to include:
typedef struct {
    PhysicalPage pages[NUM_PHYSICAL_PAGES];
    int free_page_count;
    int clock_pointer;                       // For Second Chance algorithm
    
    // Free page linked list
    FreePageNode* free_list_head;           // Head of free page list
    
    // Statistics
    int page_faults;
    int disk_reads;
    int disk_writes;
    int page_replacements;
} MemoryManager;

typedef struct {
    int free;       // 1 = free, 0 = used
    int pid;        // owning process
    int vpn;        // virtual page number
    int ref;        // for Second Chance
    int dirty;      // modified page
    int isTable;    // 1 if this frame holds a page table
} Frame;





void init_memory(void);
int allocate_free_page(int process_id, int virtual_page);
void free_process_pages(int process_id);
int second_chance_replacement(int requesting_process_id);
void handle_page_fault(int process_id, int virtual_address, char rw_flag, PCB* pcb);
void load_page_from_disk(int process_id, int virtual_page, int physical_page);
void swap_page_to_disk(int process_id, int virtual_page, int physical_page);
int translate_address(int process_id, int virtual_address, PCB* pcb, char rw_flag);
void update_disk_operations(int current_time);
bool is_page_in_disk_queue(int process_id, int virtual_page);
void print_memory_log(const char* format, ...);
void print_memory_status(void);
int init_process_page_table(PCB* pcb);
void print_free_list();
void add_to_free_list(int page_number);
int remove_from_free_list();

static inline int get_vpn(int virtual_address) {
    return virtual_address >> OFFSET_BITS;
}

static inline int get_offset(int virtual_address) {
    return virtual_address & (PAGE_SIZE - 1);
}

static inline int get_physical_address(int physical_page, int offset) {
    return (physical_page << OFFSET_BITS) | offset;
}

#endif // MMU_H