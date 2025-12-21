#ifndef MMU_H
#define MMU_H

#include "headers.h"

// Forward declaration
typedef struct PCB_struct PCB;

typedef struct FreePageNode
{
    int page_number;
    struct FreePageNode *next;
} FreePageNode;

typedef struct
{
    bool is_free;
    int process_id;
    int virtual_page_number;
    bool referenced;
    bool modified;
    bool locked;
    bool is_page_table;
} PhysicalPage;

typedef struct
{
    PhysicalPage pages[NUM_PHYSICAL_PAGES];
    int free_page_count;
    int clock_pointer;
    FreePageNode *free_list_head;
    int page_faults;
    int disk_reads;
    int disk_writes;
    int page_replacements;
} MemoryManager;

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
void debug_second_chance(int current_time);

#endif