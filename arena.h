#pragma once

/*
 * Arena allocator
 *
 * Copyright 2024 amateur80lvl
 * License: LGPLv3, see LICENSE for details
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Arena Arena;
/*
 * Arena consists of regions.
 * Arena itself and all its regions are allocated with mmap
 * and are always multiple of page size.
 *
 * The first region is embedded into the arena to avoid
 * separately allocating its small structure and fully utilize
 * allocated page.
 */

Arena* create_arena(size_t capacity);
/*
 * Create new arena with desired capacity
 * or at least one page.
 *
 * The capacity of subsequent regions will be
 * same capacity unless adjusted with `set_region_capacity`.
 */

void delete_arena(Arena* arena);
/*
 * Free arena and all its regions.
 */

void set_region_capacity(Arena* arena, size_t capacity);
/*
 * Set desired capacity of newly created regions.
 */

void* _arena_alloc(Arena* arena, size_t size, size_t alignment);
/*
 * Allocate `size` bytes from the last region aligned at `alignment` boundary.
 * If the last region has no space available, allocate new region.
 *
 * This is a low-level function because its arguments
 * tell how to allocate a block.
 *
 * The following macro gets what to allocate:
 */

#define arena_alloc(arena, num_elements, element_type) \
    _arena_alloc((arena), (num_elements) * sizeof(element_type), alignof(element_type))

void* _arena_fit(Arena* arena, size_t size, size_t element_size);
/*
 * Try to find a region with sufficient free space
 * and allocate from it.
 * If no region has space available, allocate new region.
 *
 * This is a low-level function because its arguments
 * tell how to allocate a block.
 *
 * The following macro gets what to allocate:
 */

#define arena_fit(arena, num_elements, element_type) \
    _arena_fit((arena), (num_elements) * sizeof(element_type), alignof(element_type))

void arena_print(Arena* arena);

#ifdef __cplusplus
}
#endif
