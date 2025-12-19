#include "mmu.h"
#include "headers.h"
#include "headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "signal.h"
#include <stdarg.h>
typedef short bool;

MemoryManager mem_mgr;
FILE* memory_log_file;

void init_free_list() {
    mem_mgr.free_list_head = NULL;
    mem_mgr.free_page_count = 0;
    
    for (int i = NUM_PHYSICAL_PAGES - 1; i >= 0; i--) {
        FreePageNode* node = (FreePageNode*)malloc(sizeof(FreePageNode));
        if (!node) {
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



void add_to_free_list(int page_number) {
    // Validate input
    if (page_number < 0 || page_number >= NUM_PHYSICAL_PAGES) {
        printf("Error: Invalid page number %d\n", page_number);
        return;
    }
    
    // Check if page is already free (optional but good for debugging)
    if (mem_mgr.pages[page_number].is_free) {
        printf("Warning: Page %d is already marked as free\n", page_number);
        // Should we still add it to the list? Probably not
        return;
    }
    
    // Create new free list node
    FreePageNode* node = (FreePageNode*)malloc(sizeof(FreePageNode));
    if (node == NULL) {
        printf("Error: Failed to allocate free list node\n");
        return;
    }
    
    // Initialize the node
    node->page_number = page_number;
    node->next = mem_mgr.free_list_head;
    mem_mgr.free_list_head = node;
    
    // Update the PhysicalPage array
    mem_mgr.pages[page_number].is_free = true;
    mem_mgr.pages[page_number].process_id = -1;           // No owner
    mem_mgr.pages[page_number].virtual_page_number = -1;  // No mapping
    mem_mgr.pages[page_number].referenced = false;        // Reset reference bit
    mem_mgr.pages[page_number].modified = false;          // Reset dirty bit
    mem_mgr.pages[page_number].locked = false;            // Not locked
    mem_mgr.pages[page_number].is_page_table = false;     // Not a page table
    
    // Update free page count
    mem_mgr.free_page_count++;
    
    printf("Added page %d to free list. Free pages: %d\n", 
           page_number, mem_mgr.free_page_count);
}

int remove_from_free_list() {
    // Check if there are free pages
    if (mem_mgr.free_list_head == NULL) {
        printf("Error: No free pages available\n");
        return -1;  // No free pages
    }
    
    // Remove first node from linked list (LIFO order)
    FreePageNode* node = mem_mgr.free_list_head;
    int page_number = node->page_number;
    
    // Validate the page number
    if (page_number < 0 || page_number >= NUM_PHYSICAL_PAGES) {
        printf("Error: Invalid page number %d in free list\n", page_number);
        // Clean up the corrupted node
        mem_mgr.free_list_head = node->next;
        free(node);
        return -1;
    }
    
    // Verify the page is actually marked as free in array
    if (!mem_mgr.pages[page_number].is_free) {
        printf("Warning: Page %d in free list but not marked free in array\n", page_number);
        // Still remove it from list since it shouldn't be there
        mem_mgr.free_list_head = node->next;
        free(node);
        return -1;
    }
    
    // Update linked list
    mem_mgr.free_list_head = node->next;
    free(node);
    
    // Mark page as allocated in array
    mem_mgr.pages[page_number].is_free = false;
    // Note: Don't reset other fields yet - they'll be set when page is mapped
    mem_mgr.pages[page_number].process_id = -1;
    mem_mgr.pages[page_number].virtual_page_number = -1;
    mem_mgr.pages[page_number].referenced = false;
    mem_mgr.pages[page_number].modified = false;
    mem_mgr.pages[page_number].locked = false;
    mem_mgr.pages[page_number].is_page_table = false;  
    // Update free page count
    mem_mgr.free_page_count--;
    
  printf("Free Physical page %d allocated\n", page_number);
    
    // ADD THIS LINE:
    print_memory_log("Free Physical page %d allocated\n", page_number);
    
    return page_number;
}

void print_free_list() {
    printf("Free List: ");
    FreePageNode* current = mem_mgr.free_list_head;
    int count = 0;
    
    while (current != NULL) {
        printf("%d ", current->page_number);
        current = current->next;
        count++;
    }
    
    printf("(Total: %d pages)\n", count);
}


void load_page_from_disk(int process_id, int virtual_page, int physical_page, int disk_base) {
    if (physical_page < 0 || physical_page >= NUM_PHYSICAL_PAGES) return;
    
    // Calculate which disk page to load from
    int disk_page = disk_base + virtual_page;
    
    PhysicalPage* page = &mem_mgr.pages[physical_page];
    
    // Simulate disk read from specific disk page
    mem_mgr.disk_reads++;
    printf("Reading disk page %d (VPN %d) into memory page %d\n", 
           disk_page, virtual_page, physical_page);
    
    // Update page mapping
    page->process_id = process_id;
    page->virtual_page_number = virtual_page;
    page->referenced = true;
    page->modified = false;
    page->is_page_table = false;
    page->is_free = false;
    page->locked = false;
}
void swap_page_to_disk(int process_id, int virtual_page, int physical_page) {
    // Validate inputs
    if (physical_page < 0 || physical_page >= NUM_PHYSICAL_PAGES) {
        printf("Error: Invalid physical page %d in swap_page_to_disk\n", physical_page);
        return;
    }
    
    PhysicalPage* page = &mem_mgr.pages[physical_page];
    
    // Log the swap
    printf("swap_page_to_disk: PID %d, VPN %d -> PP %d\n", 
           process_id, virtual_page, physical_page);
    
    if (page->modified) {
        // Dirty page - write to disk
        printf("  Page is dirty - simulated disk write\n");
        mem_mgr.disk_writes++;
        page->modified = false;  // Clear dirty bit after writing
    }
    
    // Free the page properly
    add_to_free_list(physical_page);
}

void print_memory_log(const char* format, ...) {
    // Open file in append mode
    memory_log_file = fopen("memory.log", "a");
    if (memory_log_file == NULL) {
        perror("Error opening memory.log");
        return;
    }

    va_list args;
    va_start(args, format);
    
    // Use vfprintf to write formatted string
    vfprintf(memory_log_file, format, args);
    
    va_end(args);
    fclose(memory_log_file);
    
    // Also print to console for debugging
  //  va_start(args, format);
  //  vprintf(format, args);
  //  va_end(args);
}



void init_memory() {
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        mem_mgr.pages[i].is_free = true;
        mem_mgr.pages[i].process_id = -1;
        mem_mgr.pages[i].virtual_page_number = -1;
        mem_mgr.pages[i].referenced = false;
        mem_mgr.pages[i].modified = false;
        mem_mgr.pages[i].locked = false;
    }
    
    mem_mgr.free_page_count = NUM_PHYSICAL_PAGES;
    mem_mgr.clock_pointer = 0;
    
    
    mem_mgr.page_faults = 0;
    mem_mgr.disk_reads = 0;
    mem_mgr.disk_writes = 0;
    mem_mgr.page_replacements = 0;
    
    
    init_free_list();
    
    
    memory_log_file = fopen("memory.log", "w");
    if (memory_log_file == NULL) {
        perror("Error opening memory.log");
        exit(1);
    }
    
  //  fprintf(memory_log_file, "#Memory Management Log\n");
  //  fprintf(memory_log_file, "#Format: Time | Event | Process | VPN | PPN | Details\n\n");
    fclose(memory_log_file);
    
    printf("Memory Manager initialized: %d pages available\n", NUM_PHYSICAL_PAGES);
}


void free_process_pages(int process_id, PCB* pcb) {
    int freed_count = 0;
    
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        if (!mem_mgr.pages[i].is_free && mem_mgr.pages[i].process_id == process_id) {
            int vpn = mem_mgr.pages[i].virtual_page_number;
            
            if (mem_mgr.pages[i].modified) {
                swap_page_to_disk(process_id, vpn, i);
            }

            // Update page table entry
            if (vpn >= 0 && vpn < pcb->page_table.num_pages) {
                PageTableEntry* entry = &pcb->page_table.entries[vpn];
                entry->present = false;
                entry->physical_page_number = -1;
            }

            // Free the page
            add_to_free_list(i);  // This increments free_page_count
            freed_count++;
        }
    }
    
    // Free page table memory
    if (pcb->page_table.entries) {
        free(pcb->page_table.entries);
        pcb->page_table.entries = NULL;
    }
    
    printf("Freed %d pages from process %d\n", freed_count, process_id);
}
int init_process_page_table(PCB* pcb) {
    if (pcb->num_pages <= 0) return -1;
    
    pcb->page_table.entries = (PageTableEntry*)calloc(pcb->num_pages, sizeof(PageTableEntry));
    if (!pcb->page_table.entries) return -1;
    
    for (int i = 0; i < pcb->num_pages; i++) {
        pcb->page_table.entries[i].present = false;
        pcb->page_table.entries[i].modified = false;
        pcb->page_table.entries[i].referenced = false;
        pcb->page_table.entries[i].physical_page_number = -1;
    }
    
    pcb->page_table.num_pages = pcb->num_pages;
    pcb->page_table.disk_base = pcb->disk_base;
    pcb->page_table.physical_page_number = -1;  // Will be set by allocate_process_page_table
    
    return 0;
}

int allocate_free_page(int process_id, int virtual_page) {
    // Try to get a free page
    int ppn = remove_from_free_list();
    if (ppn == -1) return -1;  // No free pages
    
    // IMPORTANT: remove_from_free_list() already:
    // 1. Removed from linked list
    // 2. Set page->is_free = false
    // 3. Decremented free_page_count
    // 4. Reset all page fields
    
    PhysicalPage *p = &mem_mgr.pages[ppn];
    
    // Now set up the new mapping
    p->process_id = process_id;
    p->virtual_page_number = virtual_page;
    p->referenced = true;      // Page is being referenced
    p->modified = false;       // Starts clean
    p->locked = false;         // Not locked
    p->is_page_table = false;  // Regular data page
    
    // Log the allocation (optional)
    printf("Allocated free page %d for process %d (VPN %d)\n", 
           ppn, process_id, virtual_page);
    
    return ppn;
}





int second_chance_replacement() {
    int start_pointer = mem_mgr.clock_pointer;
    int iterations = 0;
    int max_iterations = NUM_PHYSICAL_PAGES * 2;  // Safety limit
    
    while (1) {
        if (iterations >= max_iterations) {
            printf("Error: Second chance algorithm stuck in infinite loop!\n");
            return -1;
        }
        iterations++;

        PhysicalPage *page = &mem_mgr.pages[mem_mgr.clock_pointer];

        if (page->is_free) {
            mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
            if (mem_mgr.clock_pointer == start_pointer) break;
            continue;
        }

        if (page->is_page_table) {
            mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
            if (mem_mgr.clock_pointer == start_pointer) break;
            continue;
        }

        if (page->locked) {
            mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
            if (mem_mgr.clock_pointer == start_pointer) break;
            continue;
        }

        if (page->referenced) {
            page->referenced = false;
            mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
            if (mem_mgr.clock_pointer == start_pointer) break;
            continue;
        }

        int victim = mem_mgr.clock_pointer;
        mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
        mem_mgr.page_replacements++;
        return victim;
    }
    
    printf("Error: No victim found after checking all pages\n");
    return -1;
}

int translate_address(int process_id, int virtual_address, PCB* pcb, char rw_flag) {
    int vpn = (virtual_address >> OFFSET_BITS) & 0x3F;
    int offset = virtual_address & 0x0F;

    if (vpn >= pcb->page_table.num_pages) return -1;

    PageTableEntry *pte = &pcb->page_table.entries[vpn];

    if (!pte->present) return -1;

    // update ref and modified
    pte->referenced = true;
    PhysicalPage *frame = &mem_mgr.pages[pte->physical_page_number];
    frame->referenced = true;
    if (rw_flag == 'w') {
        pte->modified = true;
        frame->modified = true;
    }
    return (pte->physical_page_number << OFFSET_BITS) | offset;
}

void allocate_page_table(PCB *pcb) {
    int ppn = allocate_free_page(pcb->process_id, -1);

    if (ppn == -1) {
        ppn = second_chance_replacement();

        if (ppn == -1) {
            printf("Error: No victim found for page table allocation!\n");
            return;
        }

        // --- update victim ---
        // PhysicalPage *victim = &mem_mgr.pages[ppn];
        // if (!victim->is_page_table && victim->process_id != -1) {
        //     PCB *victim_pcb = get_pcb();
        //     if (victim_pcb) {
        //         int vpn = victim->virtual_page_number;
        //         victim_pcb->page_table.entries[vpn].present = false;
        //         victim_pcb->page_table.entries[vpn].physical_page_number = -1;
        //     }
        //     if (victim->modified) {
        //         swap_page_to_disk(victim->process_id, victim->virtual_page_number, ppn);
        //         victim->modified = false;
        //         mem_mgr.disk_writes++;
        //     }
        // }
    }

    // update ppn for page table
    pcb->page_table.physical_page_number = ppn;

    // update physical page for page table
    PhysicalPage *page = &mem_mgr.pages[ppn];
    page->is_free = false;
    page->process_id = pcb->process_id;
    page->virtual_page_number = -1;
    page->is_page_table = true;
    page->referenced = false;
    page->modified = false;
    page->locked = false;
}

void handle_page_fault(PCB *pcb, int process_Count, int process_id, int virtual_page, char readwrite_flag) {
      PCB* current_pcb = get_pcb(pcb, process_Count, process_id);
    if (!current_pcb) return;
    
    // Get disk base from page table
    int disk_base = current_pcb->page_table.disk_base;

    // Try to allocate a free page
    int frame_index = allocate_free_page(process_id, virtual_page);
    bool used_free_page = (frame_index != -1);
    
    if (!used_free_page) {
        // No free pages - use second chance replacement
        frame_index = second_chance_replacement();
        // DON'T increment page_faults here - Request() already did
        
        if (frame_index == -1) {
            printf("Error: Could not find victim page for replacement\n");
            return;
        }

        PhysicalPage* victim_frame = &mem_mgr.pages[frame_index];
        
        if (victim_frame->process_id != -1) {
            PCB* victim_pcb = get_pcb(pcb, process_Count, victim_frame->process_id);
            if (victim_pcb && victim_frame->virtual_page_number >= 0) {
                if (victim_frame->virtual_page_number < victim_pcb->page_table.num_pages) {
                    PageTableEntry* victim_pte = &victim_pcb->page_table.entries[victim_frame->virtual_page_number];
                    victim_pte->present = false;
                    victim_pte->physical_page_number = -1;
                }
            }

            if (victim_frame->modified) {
                print_memory_log("Swapping out page %d to disk\n", frame_index);
                swap_page_to_disk(victim_frame->process_id, 
                                 victim_frame->virtual_page_number, 
                                 frame_index);
            }
        }
    }
    // Note: Free page logging is done in remove_from_free_list()
    
    // Load the page into memory
    load_page_from_disk(process_id, virtual_page, frame_index);
    
    // Update current process's page table entry
    if (virtual_page >= 0 && virtual_page < current_pcb->page_table.num_pages) {
        PageTableEntry* entry = &current_pcb->page_table.entries[virtual_page];
        entry->physical_page_number = frame_index;
        entry->present = true;
        entry->referenced = true;
        entry->modified = (readwrite_flag == 'W' || readwrite_flag == 'w');
    }
    
    // Log the page loading
    print_memory_log("At time %d page %d for process %d is loaded into memory page %d.\n", 
                     getClk(), virtual_page, process_id, frame_index);
}

int Request(PCB* pcb, int process_count, int process_id, int virtual_page, char readwrite_flag) {
    PCB* current_pcb = get_pcb(pcb, process_count, process_id);
    if (!current_pcb) return -1;
    
    if (virtual_page < 0 || virtual_page >= current_pcb->page_table.num_pages) {
        printf("Error: Virtual page %d out of range for process %d\n", 
               virtual_page, process_id);
        return -1;
    }
    
    PageTableEntry* pte = &current_pcb->page_table.entries[virtual_page];
    
    if (pte->present) {
        pte->referenced = true;
        
        if (pte->physical_page_number >= 0 && 
            pte->physical_page_number < NUM_PHYSICAL_PAGES) {
            mem_mgr.pages[pte->physical_page_number].referenced = true;
        }
        
        if (readwrite_flag == 'W' || readwrite_flag == 'w') {
            pte->modified = true;
            
            if (pte->physical_page_number >= 0 && 
                pte->physical_page_number < NUM_PHYSICAL_PAGES) {
                mem_mgr.pages[pte->physical_page_number].modified = true;
            }
        }
        return 0; // No page fault
    }
    
    // PAGE FAULT
    int virtual_address = virtual_page * PAGE_SIZE;
    print_memory_log("PageFault upon VA %d from process %d\n", 
                     virtual_address, process_id);
    mem_mgr.page_faults++;  // Count the page fault
    
    handle_page_fault(current_pcb, process_count, process_id, virtual_page, readwrite_flag);
    return 1; // Page fault occurred
}