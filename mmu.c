#include "mmu.h"
#include "headers.h"
#include "headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
typedef short bool;

MemoryManager mem_mgr;
FILE* memory_log_file;


void init_memory() {
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        mem_mgr.pages[i].is_free = true;
        mem_mgr.pages[i].process_id = -1;
        mem_mgr.pages[i].virtual_page_number = -1;
        mem_mgr.pages[i].referenced = false;
        mem_mgr.pages[i].modified = false;
    }
    
    mem_mgr.free_page_count = NUM_PHYSICAL_PAGES;
    mem_mgr.clock_pointer = 0;  
   
    memory_log_file = fopen("memory.log", "w");
    if (memory_log_file == NULL) {
        perror("Error opening memory.log");
        exit(1);
    }
    
    fprintf(memory_log_file, "#Memory Management Log\n");
    fprintf(memory_log_file, "#Format: PageFault/Allocation/Swapping events\n\n");
    fclose(memory_log_file);
    
    printf("Memory Manager initialized: %d pages available\n", NUM_PHYSICAL_PAGES);
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
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        if (mem_mgr.pages[i].is_free) {
            mem_mgr.pages[i].is_free = false;
            mem_mgr.pages[i].process_id = process_id;
            mem_mgr.pages[i].virtual_page_number = virtual_page;
            mem_mgr.pages[i].referenced = true;
            mem_mgr.pages[i].modified = false;
            mem_mgr.free_page_count--;
            printf("Allocated free page %d for process %d (VP: %d)\n", 
                   i, process_id, virtual_page);
            
            return i;
        }
    }
    
    return -1;  
}


void free_process_pages(int process_id) {
    int freed_count = 0;
    
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        if (!mem_mgr.pages[i].is_free && mem_mgr.pages[i].process_id == process_id) {
            mem_mgr.pages[i].is_free = true;
            mem_mgr.pages[i].process_id = -1;
            mem_mgr.pages[i].virtual_page_number = -1;
            mem_mgr.pages[i].referenced = false;
            mem_mgr.pages[i].modified = false;
            mem_mgr.free_page_count++;
            freed_count++;
        }
    }
    
    printf("Freed %d pages from process %d\n", freed_count, process_id);
}

void print_memory_log(const char* format, ...){

}
void load_page_from_disk(int process_id, int virtual_page, int physical_page){}
void swap_page_to_disk(int process_id, int virtual_page, int physical_page){}
int second_chance_replacement(int requesting_process_id){}


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

