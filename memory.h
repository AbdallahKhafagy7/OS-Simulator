#ifndef MEMORY_STRUCTURES_H
#define MEMORY_STRUCTURES_H

typedef short bool;

#define ADDRESS_BITS 10
#define PAGE_SIZE 16
#define RAM_SIZE 512
#define NUM_PHYSICAL_PAGES 32
#define DISK_ACCESS_TIME 10
#define OFFSET_BITS 4
#define VPN_BITS 6
#define PPN_BITS 5
#define MAX_VIRTUAL_PAGES 64
#define MAX_PHYSICAL_PAGES 32

typedef struct
{
    bool present;
    bool modified;
    bool referenced;
    int physical_page_number;
} PageTableEntry;

typedef struct
{
    PageTableEntry *entries;
    int num_pages;
    int physical_page_number;
    int disk_base;
} ProcessPageTable;

typedef struct request {
    int time;
    int address;
    char rw;
} request;

#endif