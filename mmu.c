#include "mmu.h"
#include "headers.h"
#include "headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "signal.h"
typedef short bool;

MemoryManager mem_mgr;
FILE* memory_log_file;

void init_free_list() {
    mem_mgr.free_list_head = NULL;
    for (int i = NUM_PHYSICAL_PAGES - 1; i >= 0; i--) {
        FreePageNode* node = (FreePageNode*)malloc(sizeof(FreePageNode));
        node->page_number = i;
        node->next = mem_mgr.free_list_head;
        mem_mgr.free_list_head = node;
    }
    
    printf("Free list initialized with %d pages\n", NUM_PHYSICAL_PAGES);
}

void add_to_free_list(int page_number) {
    FreePageNode* node = (FreePageNode*)malloc(sizeof(FreePageNode));
    if (node == NULL) {
        printf("Error: Failed to allocate free list node\n");
        return;
    }
    
    node->page_number = page_number;
    node->next = mem_mgr.free_list_head;
    mem_mgr.free_list_head = node;
    
    printf("Added page %d to free list\n", page_number);
}

int remove_from_free_list() {
    if (mem_mgr.free_list_head == NULL) {
        return -1;  // No free pages
    }
    
    FreePageNode* node = mem_mgr.free_list_head;
    int page_number = node->page_number;
    
    mem_mgr.free_list_head = node->next;
    free(node);
    
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
    
    fprintf(memory_log_file, "#Memory Management Log\n");
    fprintf(memory_log_file, "#Format: Time | Event | Process | VPN | PPN | Details\n\n");
    fclose(memory_log_file);
    
    printf("Memory Manager initialized: %d pages available\n", NUM_PHYSICAL_PAGES);
}


void free_process_pages(int process_id) {
    int freed_count = 0;
    
    printf("\n=== Freeing pages for process %d ===\n", process_id);
    
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        if (!mem_mgr.pages[i].is_free && mem_mgr.pages[i].process_id == process_id) {
            printf("Freeing page %d (VP: %d)\n", i, mem_mgr.pages[i].virtual_page_number);
            
            
            mem_mgr.pages[i].is_free = true;
            mem_mgr.pages[i].process_id = -1;
            mem_mgr.pages[i].virtual_page_number = -1;
            mem_mgr.pages[i].referenced = false;
            mem_mgr.pages[i].modified = false;
            mem_mgr.pages[i].locked = false;
            
            
            add_to_free_list(i);
            
            mem_mgr.free_page_count++;
            freed_count++;
        }
    }
    
    printf("Freed %d pages from process %d\n", freed_count, process_id);
    printf("Free pages now: %d\n\n", mem_mgr.free_page_count);
    
    
    memory_log_file = fopen("memory.log", "a");
    if (memory_log_file) {
        fprintf(memory_log_file, "Time %d: FREE | P%d | Freed %d pages | Free total: %d\n",
                getClk(), process_id, freed_count, mem_mgr.free_page_count);
        fclose(memory_log_file);
    }
}

int init_process_page_table(PCB* pcb) {
    pcb->page_table.entries = (PageTableEntry*)calloc(pcb->num_pages, sizeof(PageTableEntry));
    
    if (pcb->page_table.entries == NULL) {
        printf("Error: Failed to allocate page table for process %d\n", pcb->process_id);
        return -1;
    }
    
     
    for (int i = 0; i < pcb->num_pages; i++) {
        pcb->page_table.entries[i].present = false;
        pcb->page_table.entries[i].modified = false;
        pcb->page_table.entries[i].referenced = false;
        pcb->page_table.entries[i].physical_page_number = 0;
    }
    
    pcb->page_table.num_pages = pcb->num_pages;
    pcb->page_table.disk_base = pcb->disk_base;
    pcb->page_table.physical_page_number = -1;  
    
    printf("Page table initialized for process %d (%d pages)\n", 
           pcb->process_id, pcb->num_pages);
    
    return 0;
}

int allocate_free_page(int process_id, int virtual_page) {
    int ppn = remove_from_free_list();
    if (ppn == -1) return -1;

    PhysicalPage *p = &mem_mgr.pages[ppn];
    p->is_free = false;
    p->process_id = process_id;
    p->virtual_page_number = virtual_page;
    p->referenced = true;
    p->modified = false;
    p->locked = false;

    mem_mgr.free_page_count--;
    return ppn;
}



void print_memory_log(const char* format, ...){

}
void load_page_from_disk(int process_id, int virtual_page, int physical_page){}

void swap_page_to_disk(int process_id, int virtual_page, int physical_page){}

int second_chance_replacement() {
    int victim_frame_index;
    PhysicalPage* page;

    while (1) {
        page = &mem_mgr.pages[mem_mgr.clock_pointer];

        if (page->is_free) {
            mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
            return mem_mgr.clock_pointer; // free page found
        }

        if (page->referenced == 1) {
			mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
            page->referenced = 0; // second chance
			continue;
        }

        if (page->referenced == 0) {
			victim_frame_index = mem_mgr.clock_pointer; // victim found
            break;
        }
    }
	mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES; // increase clock hand for next time
	return victim_frame_index;
}

void handle_page_fault(PCB *pcb, int process_Count ,int process_id, int virtual_page, char readwrite_flag) {
    
    // 1. Try to get a free page first
    int frame_index = allocate_free_page(process_id, virtual_page);
    
    if (frame_index != -1) {
        print_memory_log("Free Physical page %d allocated\n", frame_index);
        print_memory_log("At time %d page %d for process %d is loaded into memory page %d.\n", 
                         getClk(), virtual_page, process_id, frame_index);
    } 
    else { // No free page, need to replace
        frame_index = second_chance_replacement(process_id);
        
        // sigal to process  to block the 10ms until page is loaded
        if (frame_index == -1) {
            printf("Error: MMU failed to find a victim page.\n");
            return;
        }

        PhysicalPage* victim_frame = &mem_mgr.pages[frame_index];
        
        if (victim_frame->process_id != -1) {
            PCB* victim_pcb = get_pcb(pcb,process_Count, victim_frame->process_id);
            if (victim_pcb) {
                int victim_vpn = victim_frame->virtual_page_number;
                victim_pcb->page_table.entries[victim_vpn].present = false; 
                victim_pcb->page_table.entries[victim_vpn].physical_page_number = -1;
            }

            if (victim_frame->modified) {
                
                print_memory_log("Swapping out page %d to disk\n", frame_index);
                printf("Swapping out page %d to disk\n", frame_index);
                swap_page_to_disk(victim_frame->process_id, victim_frame->virtual_page_number, frame_index);
            }
        }

        print_memory_log("PageFault upon VA %d from process %d\n", virtual_page * PAGE_SIZE, process_id);
        printf("PageFault upon VA %d from process %d\n", virtual_page * PAGE_SIZE, process_id);
        
        load_page_from_disk(process_id, virtual_page, frame_index);
        
        print_memory_log("At time %d page %d for process %d is loaded into memory page %d.\n", 
                         getClk(), virtual_page, process_id, frame_index);
        printf("At time %d page %d for process %d is loaded into memory page %d.\n", 
                         getClk(), virtual_page, process_id, frame_index);

        // Update Physical Frame Data
        victim_frame->process_id = process_id;
        victim_frame->virtual_page_number = virtual_page;
        victim_frame->referenced = true; // Use 'referenced' from mmu.h
        victim_frame->modified = (readwrite_flag == 'W') ? true : false;
        victim_frame->is_free = false;
    }

    PCB* current_pcb = get_pcb(pcb, process_Count, process_id);

    if (current_pcb) {
        PageTableEntry* entry = &current_pcb->page_table.entries[virtual_page];
        
        entry->physical_page_number = frame_index;
        entry->present = true;       
        entry->referenced = true;
        
        if (readwrite_flag == 'W') {
            entry->modified = true; 
        }
    }

}

void signal_page_loaded(int PID, int virtual_page) {
   kill(PID, SIGUSR2);
}