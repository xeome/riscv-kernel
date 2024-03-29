#include "kernel.h"

extern char __free_ram[], __free_ram_end[];

struct free_list page_list;

void init_free_list(struct free_list* free_list) {
    // Initialize the free list
    for (size_t i = 0; i < NUM_PAGES; i++) {
        free_list->page_frame_addr[i] = (paddr_t)__free_ram + i * PAGE_SIZE;
    }
    free_list->page_frame_free = 0;
}

/**
 * Allocates n pages from the free list. if you are going to fragment the free list, do not allocate more than 1 page.
 *
 * @param n The number of pages to allocate.
 * @return The physical address of the first page.
 * @throws PANIC if there is not enough memory available.
 */
paddr_t alloc_page(struct free_list* free_list, size_t n) {
    if (free_list->page_frame_free >= NUM_PAGES)
        PANIC("out of memory");
    if (free_list->page_frame_addr[free_list->page_frame_free] == 0) {
        PANIC("page frame address is 0");
    }

    const paddr_t paddr = free_list->page_frame_addr[free_list->page_frame_free];
    for (size_t i = 0; i < n; i++) {
        if (free_list->page_frame_addr[free_list->page_frame_free + i] == 0)
            PANIC("page frame address is 0");
        free_list->page_frame_addr[free_list->page_frame_free + i] = 0;
    }

    free_list->page_frame_free += n;
    return paddr;
}

void free_page(struct free_list* free_list, paddr_t paddr) {
    if (free_list->page_frame_free == 0)
        PANIC("free list is empty");

    free_list->page_frame_free--;
    free_list->page_frame_addr[free_list->page_frame_free] = paddr;
}

/**
 * Maps a physical page to a virtual address in the kernel page table.
 *
 * @param table1 Pointer to the first level page table.
 * @param vaddr Virtual address to map the physical page to.
 * @param paddr Physical address of the page to map.
 * @param flags Flags to set for the page table entry.
 */
void map_page(uint32_t* table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);

    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr);

    const uint32_t vpn1 = (vaddr >> 22) & 0x3ff;  // shift 22 bits to right and mask 10 bits to extract vpn1 (First 10 bits)
    if ((table1[vpn1] & PAGE_V) == 0) {
        // Create a second level page table
        const uint32_t pt_paddr = alloc_page(&page_list, 1);     // Allocate a physical page for the second level page table
        table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;  // Set the PPN (Page Physical Number) and V (Valid) bit
    }

    // Add an entry to the second level page table
    const uint32_t vpn0 = (vaddr >> 12) & 0x3ff;  // shift 12 bits to right and mask 10 bits to extract vpn0 (Next 10 bits)
    uint32_t* table0 = (uint32_t*)((table1[vpn1] >> 10) * PAGE_SIZE);  // Get the physical address of the second level page table
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;       // Set the PPN (Page Physical Number) and V (Valid) bit
}
