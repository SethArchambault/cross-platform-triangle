#include <sys/mman.h>
#include <unistd.h> // page size
                    
Arena * arena_init() {
    U64 header_size = 64;
    U8 * ptr = (U8 *)mmap(NULL, header_size + ARENA_MAX, PROT_READ|PROT_WRITE, 
            MAP_PRIVATE|MAP_ANON, -1, 0); 
    Arena * arena = (Arena *)ptr;
    arena->cap = ARENA_MAX;
    arena->alloc_pos = 0;
    arena->commit_pos = 0;
    arena->mem = ptr + header_size;

    // maybe need to align bytes before I can use memory positioning
     //__asan_poison_memory_region(arena->mem, ARENA_MAX);

    return arena;
}

void arena_alloc(Arena * arena, U64 size) {
    U64 start = arena->alloc_pos;
    arena->alloc_pos = arena->commit_pos;
    arena->commit_pos += size;
    // maybe need to align bytes before I can use memory positioning
    // I think I have to get the alloc before hand..
    //__asan_unpoison_memory_region(arena->mem + arena->alloc_pos, size);
    // assert needed check against cap
}

