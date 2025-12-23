#include "mmu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

MemoryManager mem_mgr;
FILE *memory_log_file = NULL;
int selected_Page_Replacement_NUM_mmu = -1;

void set_page_replacement_algorithm(int algo_num)
{
    selected_Page_Replacement_NUM_mmu=algo_num;
    printf("Page Replacement Algorithm set to %d\n", selected_Page_Replacement_NUM_mmu);

}


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

void add_to_free_list(int page_number)
{
    if (page_number < 0 || page_number >= NUM_PHYSICAL_PAGES)
        return;

    FreePageNode *node = malloc(sizeof(FreePageNode));
    if (!node) return;

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
}

int remove_from_free_list()
{
    if (mem_mgr.free_list_head == NULL)
        return -1;

    FreePageNode *node = mem_mgr.free_list_head;
    int page_number = node->page_number;

    if (page_number < 0 || page_number >= NUM_PHYSICAL_PAGES)
    {
        mem_mgr.free_list_head = node->next;
        free(node);
        return -1;
    }

    mem_mgr.free_list_head = node->next;
    free(node);

    mem_mgr.pages[page_number].is_free = false;
    mem_mgr.free_page_count--;

    return page_number;
}

void init_free_list()
{
    mem_mgr.free_list_head = NULL;
    mem_mgr.free_page_count = 0;

    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++)  
    {
        add_to_free_list(i);
    }
}

void load_page_from_disk(int process_id, int virtual_page,
                         int physical_page, int disk_base)
{
    if (physical_page < 0 || physical_page >= NUM_PHYSICAL_PAGES)
        return;

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
        return;

    PhysicalPage *page = &mem_mgr.pages[physical_page];

    if (page->is_free)
        return;

    if (page->modified)
    {
        mem_mgr.disk_writes++;
    }
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
    
    int is_comment = 0;
    const char *temp = format;
    

    if (strstr(format, "PageFault") != NULL) {
        fprintf(memory_log_file, "#");
        is_comment = 1;
    }
    
    if (strstr(format, "Free Physical page") != NULL) {
        is_comment = 0;
    } else if (strstr(format, "Swapping out") != NULL) {
        is_comment = 0;
    } else if (strstr(format, "At time") != NULL && strstr(format, "loaded into memory") != NULL) {
        is_comment = 0;
    }
    
    if (is_comment && format[0] != '#') {
        fprintf(memory_log_file, "#");
    }
    
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
    FILE *clear_log = fopen("memory.log", "w");
    if (clear_log) fclose(clear_log);

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
    mem_mgr.free_list_head = NULL;

    init_free_list();
}

void free_process_pages(int process_id, PCB *pcb)
{
    int freed_count = 0;

    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++)
    {
        if (!mem_mgr.pages[i].is_free && mem_mgr.pages[i].process_id == process_id)
        {
            int virtualPageNumber = mem_mgr.pages[i].virtual_page_number;

            if (mem_mgr.pages[i].modified)
            {
                swap_page_to_disk(process_id, virtualPageNumber, i);
            }

            if (virtualPageNumber >= 0 && virtualPageNumber < pcb->page_table.num_pages)
            {
                PageTableEntry *entry = &pcb->page_table.entries[virtualPageNumber];
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
    if (mem_mgr.free_list_head == NULL)
        return -1;
    
    FreePageNode *node = mem_mgr.free_list_head;
    int ppn = node->page_number;
    
    if (ppn < 0 || ppn >= NUM_PHYSICAL_PAGES)
    {
        mem_mgr.free_list_head = node->next;
        free(node);
        return -1;
    }

    mem_mgr.free_list_head = node->next;
    free(node);

    mem_mgr.pages[ppn].is_free = false;
    mem_mgr.free_page_count--;

    PhysicalPage *p = &mem_mgr.pages[ppn];

    p->process_id = process_id;
    p->virtual_page_number = virtual_page;
    p->referenced = true;
    p->modified = false;
    p->locked = false;
    p->is_page_table = false;

    print_memory_log("Free Physical page %d allocated\n", ppn);

    return ppn;
}







int LRU_replacement()
{
    int page_to_be_removed = -1;

   for(int i = 0; i < NUM_PHYSICAL_PAGES; i++)    {
     
    
    if (mem_mgr.pages[i].is_free){
        int j;
    for(j = 0; j < NUM_PHYSICAL_PAGES; j++)
    {
        if (mem_mgr.pages[j].is_free)
        {
            mem_mgr.pages[j].LRU_counter = 1;
             page_to_be_removed = j;
        }
        else
        {
            mem_mgr.pages[j].LRU_counter++;
        }
    }
    return page_to_be_removed; 
    }
    }

    
    int min_lru=NUM_PHYSICAL_PAGES;
    

        for(int i = 0; i < NUM_PHYSICAL_PAGES; i++)
    {
        mem_mgr.pages[i].LRU_counter++;
    }


    for(int i = 0; i < NUM_PHYSICAL_PAGES; i++)
    {
        if (mem_mgr.pages[i].LRU_counter == 32)
        {
            min_lru = mem_mgr.pages[i].LRU_counter;
            page_to_be_removed = i;
            mem_mgr.page_replacements++;
            
        }
    }

return page_to_be_removed;
}











int second_chance_replacement()
{
    int start = mem_mgr.clock_pointer;
    int attempts = 0;
    const int MAX_ATTEMPTS = 2 * NUM_PHYSICAL_PAGES;

    while (attempts < MAX_ATTEMPTS)
    {
        PhysicalPage *page = &mem_mgr.pages[mem_mgr.clock_pointer];

        if (page->is_free || page->is_page_table || page->locked)
        {
            mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
            attempts++;
            continue;
        }

        if (page->referenced)
        {
            page->referenced = false;
            mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
            attempts++;
            continue;
        }

        int da7eya = mem_mgr.clock_pointer;
        mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
        mem_mgr.page_replacements++;
        return da7eya;
    }

    int limit = 0;
    
    while (mem_mgr.pages[mem_mgr.clock_pointer].is_page_table || mem_mgr.pages[mem_mgr.clock_pointer].locked)
    {
        mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
        limit++;
        if (limit > NUM_PHYSICAL_PAGES) {
            return -1; 
        }
    }
    
    int da7eya = mem_mgr.clock_pointer;
    mem_mgr.clock_pointer = (mem_mgr.clock_pointer + 1) % NUM_PHYSICAL_PAGES;
    mem_mgr.page_replacements++;
    return da7eya;

}

int handle_page_fault(PCB *pcb, int process_count, int process_id,
                      int virtual_page, char readwrite_flag, int current_time)
{
    PCB *current_pcb = get_pcb(pcb, process_count, process_id);
    if (!current_pcb)
        return 0;

    int disk_base = current_pcb->page_table.disk_base;
    
    char binary_addr[11];
    int va = virtual_page * PAGE_SIZE;
    for (int i = 9; i >= 0; i--) {
        binary_addr[9-i] = ((va >> i) & 1) ? '1' : '0';
    }
    binary_addr[10] = '\0';
    
    print_memory_log("PageFault upon VA %s from process %d\n", binary_addr, process_id);
    mem_mgr.page_faults++;

    int frame_index = -1;
    bool used_replacement = false;
    bool victim_modified = false;
    
    
    frame_index = allocate_free_page(process_id, virtual_page);
    
    if (frame_index == -1)
    {
        used_replacement = true;
        if (selected_Page_Replacement_NUM_mmu==2){
            frame_index = LRU_replacement();
        }
        else{
        frame_index = second_chance_replacement();
        }
        if (frame_index == -1)
            return 0;

        PhysicalPage *victim_frame = &mem_mgr.pages[frame_index];
        victim_modified = victim_frame->modified;
        
        if (victim_frame->process_id != -1)
        {
            PCB *victim_pcb = get_pcb(pcb, process_count, victim_frame->process_id);
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
                print_memory_log("Swapping out page %d to disk\n", frame_index);
                swap_page_to_disk(victim_frame->process_id,
                                  victim_frame->virtual_page_number,
                                  frame_index);
            }
        }
    }

    load_page_from_disk(process_id, virtual_page, frame_index, disk_base);

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

    
    int disk_page_number = disk_base + virtual_page;
    
    print_memory_log("At time %d disk address %d for process %d is loaded into memory page %d.\n",
                     current_time, disk_page_number, process_id, frame_index);

    if (used_replacement && victim_modified)
    {
        return 20; 
    }
    else
    {
        return 10; 
    }
}

int Request(PCB *pcb, int process_count, int process_id,
            int virtual_page, char readwrite_flag, int current_time)
{
    PCB *current_pcb = get_pcb(pcb, process_count, process_id);
    if (!current_pcb)
        return 0;

    if (virtual_page < 0 || virtual_page >= current_pcb->page_table.num_pages)
        return 0;

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
        return 0; 
    }

    
    return handle_page_fault(pcb, process_count, process_id, virtual_page, readwrite_flag, current_time);
}

int allocate_process_page_table(PCB *pcb)
{
    int pt_page = allocate_free_page(pcb->process_id, -1);

    if (pt_page == -1)
    {
        if (selected_Page_Replacement_NUM_mmu==2){
            pt_page = LRU_replacement();
        }
        else{
        pt_page = second_chance_replacement();
        }
        if (pt_page == -1)
            return -1;

        PhysicalPage *da7eya = &mem_mgr.pages[pt_page];
        if (da7eya->is_page_table)
            return -1;

        if (da7eya->process_id != -1 && da7eya->modified)
        {
            swap_page_to_disk(da7eya->process_id,
                              da7eya->virtual_page_number,
                              pt_page);
        }
    }

    pcb->page_table.physical_page_number = pt_page;
    mem_mgr.pages[pt_page].is_page_table = true;
    mem_mgr.pages[pt_page].virtual_page_number = -1;
    mem_mgr.pages[pt_page].process_id = pcb->process_id;
    mem_mgr.pages[pt_page].is_free = false;
    mem_mgr.pages[pt_page].locked = true;

    return pt_page;
}


int allocate_process_page_table_LRU(PCB *pcb)
{
    int pt_page = allocate_free_page(pcb->process_id, -1);

    if (pt_page == -1)
    {
       
            pt_page = LRU_replacement();
        
       
        if (pt_page == -1)
            return -1;

        PhysicalPage *da7eya = &mem_mgr.pages[pt_page];
        if (da7eya->is_page_table)
            return -1;

        if (da7eya->process_id != -1 && da7eya->modified)
        {
            swap_page_to_disk(da7eya->process_id,
                              da7eya->virtual_page_number,
                              pt_page);
        }
    }

    pcb->page_table.physical_page_number = pt_page;
    mem_mgr.pages[pt_page].is_page_table = true;
    mem_mgr.pages[pt_page].virtual_page_number = -1;
    mem_mgr.pages[pt_page].process_id = pcb->process_id;
    mem_mgr.pages[pt_page].is_free = false;
    mem_mgr.pages[pt_page].locked = true;

    return pt_page;
}


int initiate_page_fault(PCB *pcb, int process_count, int process_id,
                        int virtual_page, char readwrite_flag, int current_time,
                        int *frame_out, bool *needs_writeback) {
    PCB *current_pcb = get_pcb(pcb, process_count, process_id);
    if (!current_pcb)
        return 0;

    int disk_base = current_pcb->page_table.disk_base;
    
    char binary_addr[11];
    int va = virtual_page * PAGE_SIZE;
    for (int i = 9; i >= 0; i--) {
        binary_addr[9-i] = ((va >> i) & 1) ? '1' : '0';
    }
    binary_addr[10] = '\0';
    
    print_memory_log("PageFault upon VA %s from process %d\n", binary_addr, process_id);
    mem_mgr.page_faults++;

    int frame_index = -1;
    bool victim_modified = false;
    
   
    int free_pages = mem_mgr.free_page_count;
    
    if (free_pages > 0) {
        frame_index = allocate_free_page(process_id, virtual_page);
        
        if (frame_index == -1) {
            printf("[DEBUG] allocate_free_page failed but free_pages = %d\n", free_pages);
            for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
                if (mem_mgr.pages[i].is_free) {
                    frame_index = i;
                    FreePageNode* prev = NULL;
                    FreePageNode* curr = mem_mgr.free_list_head;
                    while (curr != NULL) {
                        if (curr->page_number == frame_index) {
                            if (prev == NULL) {
                                mem_mgr.free_list_head = curr->next;
                            } else {
                                prev->next = curr->next;
                            }
                            free(curr);
                            mem_mgr.free_page_count--;
                            break;
                        }
                        prev = curr;
                        curr = curr->next;
                    }
                    
                    PhysicalPage *p = &mem_mgr.pages[frame_index];
                    p->process_id = process_id;
                    p->virtual_page_number = virtual_page;
                    p->referenced = true;
                    p->modified = false;
                    p->locked = false;
                    p->is_page_table = false;
                    p->is_free = false;
                    
                    print_memory_log("Free Physical page %d allocated (manual)\n", frame_index);
                    break;
                }
            }
        }
    }
    
    if (frame_index == -1) {
        if (selected_Page_Replacement_NUM_mmu==2){
            frame_index = LRU_replacement();
        }
        else{
            frame_index = second_chance_replacement();
        }
        
        if (frame_index == -1)
            return 0;

        PhysicalPage *victim_frame = &mem_mgr.pages[frame_index];
        victim_modified = victim_frame->modified;
        
        if (victim_frame->process_id != -1) {
            PCB *victim_pcb = get_pcb(pcb, process_count, victim_frame->process_id);
            if (victim_pcb && victim_frame->virtual_page_number >= 0) {
                if (victim_frame->virtual_page_number < victim_pcb->page_table.num_pages) {
                    PageTableEntry *victim_pte = &victim_pcb->page_table.entries[victim_frame->virtual_page_number];
                    victim_pte->present = false;
                    victim_pte->physical_page_number = -1;
                }
            }

            if (victim_frame->modified) {
                print_memory_log("Swapping out page %d to disk\n", frame_index);
            }
        }
    }

    *frame_out = frame_index;
    *needs_writeback = victim_modified;
    
    if (victim_modified) {
        return 20; 
    } else {
        return 10; 
    }
}

void complete_page_fault(PCB *pcb, int process_count, int process_id,
                         int virtual_page, int frame_index, 
                         char readwrite_flag, int current_time) {
    PCB *current_pcb = get_pcb(pcb, process_count, process_id);
    if (!current_pcb)
        return;

    int disk_base = current_pcb->page_table.disk_base;
    
    load_page_from_disk(process_id, virtual_page, frame_index, disk_base);

    if (virtual_page >= 0 && virtual_page < current_pcb->page_table.num_pages) {
        PageTableEntry *entry = &current_pcb->page_table.entries[virtual_page];
        entry->physical_page_number = frame_index;
        entry->present = true;
        entry->referenced = true;
        entry->modified = (readwrite_flag == 'W' || readwrite_flag == 'w');
        
        if (entry->modified) {
            mem_mgr.pages[frame_index].modified = true;
        }
    }

    int disk_page_number = disk_base + virtual_page;
    print_memory_log("At time %d disk address %d for process %d is loaded into memory page %d.\n",
                     current_time, disk_page_number, process_id, frame_index);
}

int Request_New(PCB *pcb, int process_count, int process_id,
                int virtual_page, char readwrite_flag, int current_time,
                int *frame_out, bool *is_page_fault) {
    PCB *current_pcb = get_pcb(pcb, process_count, process_id);
    if (!current_pcb)
        return 0;

    if (virtual_page < 0 || virtual_page >= current_pcb->page_table.num_pages)
        return 0;

    PageTableEntry *pte = &current_pcb->page_table.entries[virtual_page];

    if (pte->present) {
        *is_page_fault = false;
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
        return 0; 
    }

    *is_page_fault = true;
    bool needs_writeback;
    int disk_time = initiate_page_fault(pcb, process_count, process_id, 
                                        virtual_page, readwrite_flag, 
                                        current_time, frame_out, &needs_writeback);
    
    return disk_time;
}