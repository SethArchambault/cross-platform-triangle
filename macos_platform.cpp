#include <sys/mman.h>
#include <unistd.h> // page size

#ifdef POISON
#include <sanitizer/asan_interface.h>
#endif
                    
Arena * arena_init() {
    U64 header_size = 64;
    U8 * ptr = (U8 *)mmap(NULL, header_size + ARENA_MAX, PROT_READ|PROT_WRITE, 
            MAP_PRIVATE|MAP_ANON, -1, 0); 
    Arena * arena = (Arena *)ptr;
    arena->cap = ARENA_MAX;
    arena->alloc_pos = 0;
    arena->commit_pos = 0;
    arena->mem = ptr + header_size;

#ifdef POISON
    printf("poison\n");
    __asan_poison_memory_region(arena->mem, ARENA_MAX);
#endif 

    return arena;
}

U8 * arena_alloc(Arena * arena, U64 size) {
    U64 start = arena->alloc_pos;
    arena->alloc_pos = arena->commit_pos;
    arena->commit_pos += size + (8 - (size % 8));
#ifdef POISON
    __asan_unpoison_memory_region(arena->mem + arena->alloc_pos, size);
#endif 
    return (U8 *) arena->mem + arena->alloc_pos;
}

