/*
 * Arena allocator
 *
 * Copyright 2024 amateur80lvl
 * License: LGPLv3, see LICENSE for details
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "arena.h"

/**********************************************************
 * General purpose helper functions and macros
 */

#define is_power_of_two(v) (v && !((v & (v - 1))))
// http://www.graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2

#define max(a, b) (((a) > (b))? (a) : (b))

static inline size_t align(size_t n, size_t alignment)
/*
 * Align `n` to `alignment` boundary which must be a power of two or zero.
 */
{
    if (alignment > 0) {
        register size_t a = alignment - 1;
        return (n + a) & ~a;
    } else {
        return n;
    }
}

static inline size_t align_to_page(size_t n)
/*
 * Align `n` to page boundary.
 */
{
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    return align(n, page_size);
}

static void* alloc_mem(size_t size)
{
    void* result = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) {
        perror("Arena allocation");
        exit(1);
    }
    return result;
}

static inline void free_mem(void* ptr, size_t size)
{
    munmap(ptr, size);
}

/**********************************************************
 * Arena and Region structures
 */

typedef struct _Region Region;

struct _Region {
    Region* next;
    size_t tail;
    size_t capacity;
    char data[1];
};

#define REGION_HEADER_SIZE offsetof(Region, data)

struct _Arena {
    Region* last;
    size_t new_region_capacity;
    Region  first;  // arena embeds the first region
};

#define ARENA_HEADER_SIZE (offsetof(Arena, first) + REGION_HEADER_SIZE)


/**********************************************************
 * Internal functions
 */

static inline void free_arena(Arena* arena)
{
    free_mem(arena, arena->first.capacity + ARENA_HEADER_SIZE);
}

static inline void free_region(Region* region)
{
    free_mem(region, region->capacity + REGION_HEADER_SIZE);
}

static Region* create_region(size_t capacity)
{
    size_t mem_size = align_to_page(capacity + REGION_HEADER_SIZE);
    Region* region = alloc_mem(mem_size);
    region->next = nullptr;
    region->tail = 0;
    region->capacity = mem_size - REGION_HEADER_SIZE;
    return region;
}

static void* region_alloc(Region* region, size_t size, size_t alignment)
/*
 * Allocate aligned `size` bytes from `region`.
 * Return nullptr if `region` has no space available.
 */
{
    assert(size > 0);
    assert(alignment <= alignof(max_align_t) && is_power_of_two(alignment));

    size_t start = align(region->tail, alignment);
    if (start >= region->capacity) {
        return nullptr;
    }

    size_t bytes_available = region->capacity - start;
    if (size > bytes_available) {
        return nullptr;
    }

    void* result = &region->data[start];
    region->tail = start + size;
    return result;
}

static void* new_region_alloc(Arena* arena, size_t size, size_t alignment)
/*
 * Create new region and allocate aligned `size` bytes from it.
 */
{
    Region* new_region = create_region(max(size, arena->new_region_capacity));
    arena->last->next = new_region;
    arena->last = new_region;
    return region_alloc(new_region, size, alignment);
}

/**********************************************************
 * Public API
 */

Arena* create_arena(size_t capacity)
{
    size_t mem_size = align_to_page(capacity + ARENA_HEADER_SIZE);
    Arena* arena = (Arena*) alloc_mem(mem_size);
    arena->last = &arena->first;

    arena->first.next = nullptr;
    arena->first.tail = 0;
    arena->first.capacity = mem_size - ARENA_HEADER_SIZE;
    arena->new_region_capacity = capacity;

    return arena;
}

void delete_arena(Arena* arena)
{
    for (Region* region = arena->first.next; region != nullptr;) {

        assert(region->tail <= region->capacity);

        Region* next_region = region->next;
        free_region(region);
        region = next_region;
    }

    assert(arena->first.tail <= arena->first.capacity);

    free_arena(arena);
}

void set_region_capacity(Arena* arena, size_t capacity)
{
    arena->new_region_capacity = capacity;
}

void* _arena_alloc(Arena* arena, size_t size, size_t alignment)
{
    void* result = region_alloc(arena->last, size, alignment);
    if (result) {
        return result;
    } else {
        return new_region_alloc(arena, size, alignment);
    }
}

void* _arena_fit(Arena* arena, size_t size, size_t alignment)
{
    for (Region* region = &arena->first; region != nullptr; region = region->next) {
        void* result = region_alloc(region, size, alignment);
        if (result) {
            return result;
        }
    }
    return new_region_alloc(arena, size, alignment);
}

void arena_print(Arena* arena)
{
    printf("Arena at %p\n", arena);
    printf("last region: %p\n", arena->last);
    printf("new_region_capacity: %zu\n", arena->new_region_capacity);
    printf("first region -> next: %p\n", arena->first.next);
    printf("first region -> tail: %zu\n", arena->first.tail);
    printf("first region -> capacity: %zu\n", arena->first.capacity);
    for (Region* region = arena->first.next; region != nullptr; region = region->next) {
        printf("\nRegion %p\n", region);
        printf("next region: %p\n", region->next);
        printf("tail: %zu\n", region->tail);
        printf("capacity: %zu\n", region->capacity);
    }
}
