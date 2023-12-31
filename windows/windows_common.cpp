#define assert(expr) if(!(expr)) { printf("%s:%d %s() %s\n",__FILE__,__LINE__, __func__, #expr); *(volatile int *)0 = 0; }

Arena * arena_init() {
    U64 header_size = 64;
    void * ptr = VirtualAlloc( nullptr, header_size + ARENA_MAX, 
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); 
    assert(ptr);
    // put the reserve in arena_alloc 
    // MEM_LARGE_PAGES - could be higher memory performance
    // TODO: MEM_RESET when popping
    Arena * arena = (Arena *)ptr;
    arena->cap = ARENA_MAX;
    arena->alloc_pos = 0;
    arena->commit_pos = 0;
    arena->mem = (U8 *)ptr + header_size;
    return arena;
}

// alloc_pos is the starting point of the memoroy
// commit_post is the ending point of the memory
U8 * arena_alloc(Arena * arena, U64 size) {
    U64 start = arena->alloc_pos;
    arena->alloc_pos = arena->commit_pos;
    arena->commit_pos += size + (8 - (size % 8));
    return (U8 *) arena->mem + arena->alloc_pos;
}

