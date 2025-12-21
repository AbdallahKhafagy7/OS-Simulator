#include "mmu.h"
#include "headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// Global memory manager instance
MemoryManager mem_mgr;
FILE *memory_log_file = NULL;

// Helper functions to access memory manager statistics
int get_free_page_count(void) {
    return mem_mgr.free_page_count;
}

int get_page_faults(void) {
    return mem_mgr.page_faults;
}

int get_page_replacements(void) {
    return mem_mgr.page_replacements;
}

int get_disk_reads(void) {
    return mem_mgr.disk_reads;
}

int get_disk_writes(void) {
    return mem_mgr.disk_writes;
}

void init_free_list()
{
    mem_mgr.free_list_head = NULL;
    mem_mgr.free_page_count = 0;

    for (int i = NUM_PHYSICAL_PAGES - 1; i >= 0; i--)
    {
        FreePageNode *node = (FreePageNode *)malloc(sizeof(FreePageNode));
        if (!node)
        {
            printf("Error: Memory allocation failed for free list node\n");
            exit(1);
        }
        node->page_number = i;
        node->next = mem_mgr.free_list_head;
        mem_mgr.free_list_head = node;
        mem_mgr.free_page_count++;

        mem_mgr.pages[i].is_free = true;
        mem_mgr.pages[i].process_id = -1;
        mem_mgr.pages[i].virtual_page_number = -1;
        mem_mgr.pages[i].referenced = false;
        mem_mgr.pages[i].modified = false;
        mem_mgr.pages[i].locked = false;
        mem_mgr.pages[i].is_page_table = false;
    }

    mem_mgr.clock_pointer = 0;
    mem_mgr.page_faults = 0;
    mem_mgr.disk_reads = 0;
    mem_mgr.disk_writes = 0;
    mem_mgr.page_replacements = 0;

    printf("Free list initialized with %d pages\n", NUM_PHYSICAL_PAGES);
}

void add_to_free_list(int page_number)
{
    if (page_number < 0 || page_number >= NUM_PHYSICAL_PAGES)
    {
        printf("Error: Invalid page number %d\n", page_number);
        return;
    }

    if (mem_mgr.pages[page_number].is_free)
    {
        printf("Warning: Page %d is already marked as free\n", page_number);
        return;
    }

    FreePageNode *node = (FreePageNode *)malloc(sizeof(FreePageNode));
    if (node == NULL)
    {
        printf("Error: Failed to allocate free list node\n");
        return;
    }

    node->page_number = page_number;
    node->next = mem_mgr.free_list_head;
    mem_mgr.free_list_head = node;

    mem_mgr.pages[page_number].is_free = true;
    mem_mgr.pages[page_number].process_id = -1;
    mem_mgr.pages[page_number].virtual_page_number = -1;
    mem_mgr.pages[page_number].referenced = false;
    mem_mgr.pages[page_number].modified = false;
    mem_mgr.pages[page_number].locked = false;
    mem_mgr.pages[page_number].is_page_table = false;

    mem_mgr.free_page_count++;

    printf("Added page %d to free list. Free pages: %d\n",
           page_number, mem_mgr.free_page_count);
}

int remove_from_free_list()
{
    if (mem_mgr.free_list_head == NULL)
    {
        printf("Error: No free pages available\n");
        return -1;
    }

    FreePageNode *node = mem_mgr.free_list_head;
    int page_number = node->page_number;

    if (page_number < 0 || page_number >= NUM_PHYSICAL_PAGES)
    {
        printf("Error: Invalid page number %d in free list\n", page_number);
        mem_mgr.free_list_head = node->next;
        free(node);
        return -1;
    }

    if (!mem_mgr.pages[page_number].is_free)
    {
        printf("Warning: Page %d in free list but not marked free in array\n", page_number);
        mem_mgr.free_list_head = node->next;
        free(node);
        return -1;
    }

    mem_mgr.free_list_head = node->next;
    free(node);

    mem_mgr.pages[page_number].is_free = false;
    mem_mgr.pages[page_number].process_id = -1;
    mem_mgr.pages[page_number].virtual_page_number = -1;
    mem_mgr.pages[page_number].referenced = false;
    mem_mgr.pages[page_number].modified = false;
    mem_mgr.pages[page_number].locked = false;
    mem_mgr.pages[page_number].is_page_table = false;

    mem_mgr.free_page_count--;

    printf("Free Physical page %d allocated\n", page_number);
    print_memory_log("Free Physical page %d allocated\n", page_number);

    return page_number;
}

void print_free_list()
{
    printf("Free List: ");
    FreePageNode *current = mem_mgr.free_list_head;
    int count = 0;

    while (current != NULL)
    {
        printf("%d ", current->page_number);
        current = current->next;
        count++;
    }

    printf("(Total: %d pages)\n", count);
}

void load_page_from_disk(int process_id, int virtual_page,
                         int physical_page, int disk_base)
{
    if (physical_page < 0 || physical_page >= NUM_PHYSICAL_PAGES)
        return;

    int disk_address = (disk_base + virtual_page) * PAGE_SIZE;
    PhysicalPage *page = &mem_mgr.pages[physical_page];

    mem_mgr.disk_reads++;

    page->process_id = process_id;
    page->virtual_page_number = virtual_page;
    page->referenced = true;
    page->modified = false;
    page->is_page_table = false;
    page->is_free = false;
    page->locked = false;
}

void swap_page_to_disk(int process_id, int virtual_page, int physical_page)
{
    if (physical_page < 0 || physical_page >= NUM_PHYSICAL_PAGES)
    {
        printf("Error: Invalid physical page %d in swap_page_to_disk\n", physical_page);
        return;
    }

    PhysicalPage *page = &mem_mgr.pages[physical_page];

    if (page->is_free)
    {
        printf("Warning: Trying to swap out a free page %d\n", physical_page);
        return;
    }

    if (page->modified)
    {
        mem_mgr.disk_writes++;
        // Write to disk takes 10 cycles - simulated by blocking process
    }

    print_memory_log("Swapping out page %d to disk\n", physical_page);
}

void print_memory_log(const char *format, ...)
{
    if (memory_log_file == NULL)
    {
        memory_log_file = fopen("memory.log", "a");
        if (memory_log_file == NULL)
        {
            perror("Error opening memory.log");
            return;
        }
    }

    va_list args;
    va_start(args, format);

    vfprintf(memory_log_file, format, args);

    va_end(args);
    fflush(memory_log_file);
}

void close_memory_log(void)
{
    if (memory_log_file)
    {
        fclose(memory_log_file);
        memory_log_file = NULL;
    }
}

void init_memory()
{
    // Clear memory.log file
    FILE *clear_log = fopen("memory.log", "w");
    if (clear_log)
        fclose(clear_log);

    // Initialize memory manager
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++)
    {
        mem_mgr.pages[i].is_free = true;
        mem_mgr.pages[i].process_id = -1;
        mem_mgr.pages[i].virtual_page_number = -1;
        mem_mgr.pages[i].referenced = false;
        mem_mgr.pages[i].modified = false;
        mem_mgr.pages[i].locked = false;
        mem_mgr.pages[i].is_page_table = false;
    }

    mem_mgr.free_page_count = NUM_PHYSICAL_PAGES;
    mem_mgr.clock_pointer = 0;
    mem_mgr.page_faults = 0;
    mem_mgr.disk_reads = 0;
    mem_mgr.disk_writes = 0;
    mem_mgr.page_replacements = 0;

    init_free_list();

    printf("Memory Manager initialized: %d pages available\n", NUM_PHYSICAL_PAGES);
}

void free_process_pages(int process_id, PCB *pcb)
{
    int freed_count = 0;

    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++)
    {
        if (!mem_mgr.pages[i].is_free && mem_mgr.pages[i].process_id == process_id)
        {
            int vpn = mem_mgr.pages[i].virtual_page_number;

            if (mem_mgr.pages[i].modified)
            {
                swap_page_to_disk(process_id, vpn, i);
            }

            if (vpn >= 0 && vpn < pcb->page_table.num_pages)
            {
                PageTableEntry *entry = &pcb->page_table.entries[vpn];
                entry->present = false;
                entry->physical_page_number = -1;
            }

            add_to_free_list(i);
            freed_count++;
        }
    }

    if (pcb->page_table.entries)
    {
        free(pcb->page_table.entries);
        pcb->page_table.entries = NULL;
    }

    printf("Freed %d pages from process %d\n", freed_count, process_id);
}

int init_process_page_table(PCB *pcb)
{
    if (pcb->num_pages <= 0)
        return -1;

    pcb->page_table.entries = (PageTableEntry *)calloc(pcb->num_pages, sizeof(PageTableEntry));
    if (!pcb->page_table.entries)
        return -1;

    for (int i = 0; i < pcb->num_pages; i++)
    {
        pcb->page_table.entries[i].present = false;
        pcb->page_table.entries[i].modified = false;
        pcb->page_table.entries[i].referenced = false;
        pcb->page_table.entries[i].physical_page_number = -1;
    }

    pcb->page_table.num_pages = pcb->num_pages;
    pcb->page_table.disk_base = pcb->disk_base;
    pcb->page_table.physical_page_number = -1;

    return 0;
}

int allocate_free_page(int process_id, int virtual_page)
{
    int ppn = remove_from_free_list();
    if (ppn == -1)
        return -1;

    PhysicalPage *p = &mem_mgr.pages[ppn];

    p->process_id = process_id;
    p->virtual_page_number = virtual_page;
    p->referenced = true;
    p->modified = false;
    p->locked = false;
    p->is_page_table = false;

    return ppn;
}

int second_chance_replacement()
{
    int start = mem_mgr.clock_pointer;
    int attempts = 0;
    const int MAX_ATTEMPTS = 2 * NUM_PHYSICAL_PAGES;
    
    while (attempts < MAX_ATTEMPTS)
    {
        PhysicalPage *page = &mem_mgr.pages[mem_mgr.clock_pointer];
        
        // Skip free pages, page tables, and locked pages
        if (page->is_free || page->is_page_table || page->locked)
        {
            mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
            attempts++;
            continue;
        }
        
        // Check reference bit
        if (page->referenced)
        {
            // Give second chance: clear reference bit and move on
            page->referenced = false;
            mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
            attempts++;
            continue;
        }
        
        // Found victim with reference bit 0
        int victim = mem_mgr.clock_pointer;
        mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
        mem_mgr.page_replacements++;
        return victim;
    }
    
    // If we get here, all pages had reference bits set after two full cycles
    // Reset clock pointer and return the current page
    int victim = mem_mgr.clock_pointer;
    mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
    mem_mgr.page_replacements++;
    return victim;
}

int translate_address(int process_id, int virtual_address, PCB *pcb, char rw_flag)
{
    int vpn = (virtual_address >> OFFSET_BITS) & 0x3F;
    int offset = virtual_address & 0x0F;

    if (vpn >= pcb->page_table.num_pages)
        return -1;

    PageTableEntry *pte = &pcb->page_table.entries[vpn];

    if (!pte->present)
        return -1;

    pte->referenced = true;
    PhysicalPage *frame = &mem_mgr.pages[pte->physical_page_number];
    frame->referenced = true;
    if (rw_flag == 'w')
    {
        pte->modified = true;
        frame->modified = true;
    }
    return (pte->physical_page_number << OFFSET_BITS) | offset;
}

int handle_page_fault(PCB *pcb, int process_Count, int process_id,
                      int virtual_page, char readwrite_flag, int current_time)
{
    PCB *current_pcb = get_pcb(pcb, process_Count, process_id);
    if (!current_pcb)
    {
        printf("Error: PCB not found for process %d\n", process_id);
        return 0;
    }

    int disk_base = current_pcb->page_table.disk_base;
    
    // Log page fault with binary address
    char binary_addr[11];
    int va = virtual_page * PAGE_SIZE;
    for (int i = 9; i >= 0; i--) {
        binary_addr[9-i] = ((va >> i) & 1) ? '1' : '0';
    }
    binary_addr[10] = '\0';
    
    print_memory_log("PageFault upon VA %s from process %d\n", binary_addr, process_id);
    printf("PageFault upon VA %s from process %d\n", binary_addr, process_id);

    mem_mgr.page_faults++;

    int frame_index = -1;
    bool used_second_chance = false;
    
    // First try to allocate a free page
    frame_index = allocate_free_page(process_id, virtual_page);
    
    // If no free pages, use Second Chance algorithm
    if (frame_index == -1)
    {
        used_second_chance = true;
        frame_index = second_chance_replacement();
        
        if (frame_index == -1)
        {
            printf("Error: Could not find victim page for replacement\n");
            return 0;
        }

        PhysicalPage *victim_frame = &mem_mgr.pages[frame_index];
        
        if (victim_frame->process_id != -1)
        {
            PCB *victim_pcb = get_pcb(pcb, process_Count, victim_frame->process_id);
            if (victim_pcb && victim_frame->virtual_page_number >= 0)
            {
                if (victim_frame->virtual_page_number < victim_pcb->page_table.num_pages)
                {
                    PageTableEntry *victim_pte = &victim_pcb->page_table.entries[victim_frame->virtual_page_number];
                    victim_pte->present = false;
                    victim_pte->physical_page_number = -1;
                }
            }

            if (victim_frame->modified)
            {
                printf("Swapping out modified page %d to disk (10 cycles)\n", frame_index);
                swap_page_to_disk(victim_frame->process_id,
                                  victim_frame->virtual_page_number,
                                  frame_index);
            }
        }
    }

    // Load the page from disk
    load_page_from_disk(process_id, virtual_page, frame_index, disk_base);

    // Update page table entry
    if (virtual_page >= 0 && virtual_page < current_pcb->page_table.num_pages)
    {
        PageTableEntry *entry = &current_pcb->page_table.entries[virtual_page];
        entry->physical_page_number = frame_index;
        entry->present = true;
        entry->referenced = true;
        entry->modified = (readwrite_flag == 'W' || readwrite_flag == 'w');
        
        if (entry->modified) {
            mem_mgr.pages[frame_index].modified = true;
        }
    }

    // Calculate disk address
    int disk_address = (disk_base + virtual_page) * PAGE_SIZE;

    // Log to memory.log with exact format
    print_memory_log("At time %d disk address %d for process %d is loaded into memory page %d.\n",
                     current_time, disk_address, process_id, frame_index);
    
    // Also print to console for debugging
    printf("At time %d disk address %d for process %d is loaded into memory page %d.\n",
           current_time, disk_address, process_id, frame_index);

    // Return I/O time required
    // If we swapped out a modified page: 10 (write) + 10 (read) = 20
    // If we swapped out a clean page: 10 (read) = 10
    // If we used free page: 10 (read) = 10
    if (used_second_chance && mem_mgr.pages[frame_index].modified)
    {
        return 20; // Modified victim: write + read
    }
    else
    {
        return 10; // Clean victim or free page: just read
    }
}

int Request(PCB *pcb, int process_count, int process_id,
            int virtual_page, char readwrite_flag, int current_time)
{
    PCB *current_pcb = get_pcb(pcb, process_count, process_id);
    if (!current_pcb)
        return 0; // No page fault (error, but return 0)

    if (virtual_page < 0 || virtual_page >= current_pcb->page_table.num_pages)
    {
        printf("Error: Virtual page %d out of range for process %d\n",
               virtual_page, process_id);
        return 0;
    }

    PageTableEntry *pte = &current_pcb->page_table.entries[virtual_page];

    if (pte->present)
    {
        pte->referenced = true;

        if (pte->physical_page_number >= 0 &&
            pte->physical_page_number < NUM_PHYSICAL_PAGES)
        {
            mem_mgr.pages[pte->physical_page_number].referenced = true;
        }

        if (readwrite_flag == 'W' || readwrite_flag == 'w')
        {
            pte->modified = true;

            if (pte->physical_page_number >= 0 &&
                pte->physical_page_number < NUM_PHYSICAL_PAGES)
            {
                mem_mgr.pages[pte->physical_page_number].modified = true;
            }
        }
        return 0; // No page fault
    }

    // PAGE FAULT - call handle_page_fault and return I/O time
    return handle_page_fault(pcb, process_count, process_id, virtual_page, readwrite_flag, current_time);
}

int allocate_process_page_table(PCB *pcb)
{
    int pt_page = allocate_free_page(pcb->process_id, -1);

    if (pt_page == -1)
    {
        pt_page = second_chance_replacement();
        if (pt_page == -1)
        {
            printf("Error: Cannot allocate page for page table!\n");
            return -1;
        }

        PhysicalPage *victim = &mem_mgr.pages[pt_page];
        if (victim->is_page_table)
        {
            printf("Warning: Trying to evict page table %d\n", pt_page);
            return -1;
        }

        if (victim->process_id != -1 && victim->modified)
        {
            swap_page_to_disk(victim->process_id,
                              victim->virtual_page_number,
                              pt_page);
        }
    }

    pcb->page_table.physical_page_number = pt_page;
    mem_mgr.pages[pt_page].is_page_table = true;
    mem_mgr.pages[pt_page].virtual_page_number = -1;
    mem_mgr.pages[pt_page].process_id = pcb->process_id;
    mem_mgr.pages[pt_page].is_free = false;
    mem_mgr.pages[pt_page].locked = true; // Page table should not be swapped out

    printf("Process %d: Page table at physical page %d\n",
           pcb->process_id, pt_page);

    return pt_page;
}

void print_memory_status()
{
    printf("\n=== Memory Status ===\n");
    printf("Free pages: %d/%d\n", get_free_page_count(), NUM_PHYSICAL_PAGES);
    printf("Page faults: %d\n", get_page_faults());
    printf("Page replacements: %d\n", get_page_replacements());
    printf("Disk reads: %d\n", get_disk_reads());
    printf("Disk writes: %d\n", get_disk_writes());
    printf("Clock pointer: %d\n", mem_mgr.clock_pointer);
    
    printf("\nPhysical Page Allocation:\n");
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++)
    {
        if (!mem_mgr.pages[i].is_free)
        {
            printf("  Page %2d: PID=%2d, VPN=%2d, R=%d, M=%d, %s\n",
                   i,
                   mem_mgr.pages[i].process_id,
                   mem_mgr.pages[i].virtual_page_number,
                   mem_mgr.pages[i].referenced,
                   mem_mgr.pages[i].modified,
                   mem_mgr.pages[i].is_page_table ? "PT" : "Data");
        }
    }
    printf("====================\n");
}