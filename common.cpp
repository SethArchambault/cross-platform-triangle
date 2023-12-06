#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h> // page size
#include <sanitizer/asan_interface.h>
#include <string.h> // memcpy

/// Types ////

typedef int8_t      S8;
typedef int16_t     S16;
typedef int32_t     S32; // use this mostly
typedef int64_t     S64;
typedef int32_t     B32;

typedef uint8_t     U8;
typedef uint16_t    U16;
typedef uint32_t    U32;
typedef uint64_t    U64;

typedef float       F32;
typedef double      F64;

typedef U8          B8;


#define Gigabytes(count) (U64) (count * 1024 * 1024 * 1024)
#define Megabytes(count) (U64) (count * 1024 * 1024)
#define Kilobytes(count) (U64) (count * 1024)

#define ARENA_MAX Gigabytes(1)

/// Memory

struct Arena {
    U8 *mem;
    U64 cap;
    U64 alloc_pos;
    U64 commit_pos;
};

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

/// String

struct String {
    U64 size;
    U8 *data;
};

String * string_alloc(Arena * arena, U64 size) {
    arena_alloc(arena, size + 64);
    String * str = (String *) arena->mem + arena->alloc_pos;
    //__asan_unpoison_memory_region(str, arena->alloc_pos + 64);
    str->size  = size;
    str->data  = (U8 *)arena->mem + arena->alloc_pos + 64;
    return str;
}

String * string_make(Arena * arena, const char * raw) {
    U64 str_size = (U64) strlen(raw);
    String * str = string_alloc(arena, str_size);
    memcpy(str->data, raw, str_size);
    return str;
}

String * string_concat(Arena * arena, String *str1, String *str2) {
    U64 str3_size = str1->size + str2->size;
    String * str3 = string_alloc(arena, str3_size);
    str3->size = str3_size;
    memcpy(str3->data, str1->data, str1->size);
    memcpy(str3->data + str1->size, str2->data, str2->size);
    return str3;
}

String * string_concat(Arena * arena, String *str1, const char * raw) {
    U64 raw_size = (U64) strlen(raw);
    U64 str3_size = str1->size + raw_size;
    String * str3 = string_alloc(arena, str3_size);
    str3->size = str3_size;
    memcpy(str3->data, str1->data, str1->size);
    memcpy(str3->data + str1->size, raw, raw_size);
    return str3;
}

B8 string_compare(const char * raw1, const char * raw2) {
    U64 raw1_size = strlen(raw1);
    if (raw1_size != strlen(raw2)) return false;
    for(U32 idx = 0; idx < raw1_size; ++idx) {
        if (raw1[idx] != raw2[idx]) return false;
    }
    return true;
}

void string_print(String * str) {
    printf("%.*s\n", (int)str->size, str->data);
}




