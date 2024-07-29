# Arena allocator

## Features

* multiple regions
* allocated values are properly aligned
* `mmap` as underlying allocator

The capacity of arena itself and all its regions is always multiple
of page size.

C23 is required. Just because I love **nullptr**.
Plus, GNU extensions are required for `mmap`.

In notoriously stable Devuan this can be compiled with
```bash
clang-16 --std=gnu2x
```
(as of July 2024)

## How to use

```c
#include <stdio.h>
#include "arena.h"

int main(int argc, char* argv[])
{
    Arena *arena = create_arena(0);

    for(int i = 0; i < 500; i++) {
        arena_alloc(arena, 3, long long);
    }

    arena_fit(arena, 10, char);
    arena_fit(arena, 3000, char);

    arena_print(arena);
    delete_arena(arena);
}
```

## API

```c
Arena* create_arena(size_t capacity);
```
Create new arena with desired capacity
or at least one page.

The capacity of subsequent regions will be
same capacity unless adjusted with `set_region_capacity`.

```c
void delete_arena(Arena* arena);
```
Free arena and all its regions.

```c
void set_region_capacity(Arena* arena, size_t capacity);
```
Set desired capacity of newly created regions.

```c
void* arena_alloc(Arena* arena, size_t num_elements, element_type_name);
```
Allocate properly aligned `num_elements` from the last region.
If the last region has no space available, allocate new region.

```c
void* arena_fit(Arena* arena, size_t num_elements, element_type_name);
```
Try to find a region with sufficient free space
and allocate from it.
If no region has space available, allocate new region.
