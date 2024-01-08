#include <stdint.h>
#include <math.h>

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

/// Vectors

struct V2S32 {
    S32 x;
    S32 y;
};

struct V3S32 {
    union {
        S32 x;
        S32 r;
    };
    union {
        S32 y;
        S32 g;
    };
    union {
        S32 z;
        S32 b;
    };
};

struct V2F32 {
    F32 x;
    F32 y;
};

struct V3F32 {
    union {
        F32 x;
        F32 r;
    };
    union {
        F32 y;
        F32 g;
    };
    union {
        F32 z;
        F32 b;
    };
};

#define Gigabytes(count) (U64) (count * 1024 * 1024 * 1024)
#define Megabytes(count) (U64) (count * 1024 * 1024)
#define Kilobytes(count) (U64) (count * 1024)

#define ARENA_MAX Gigabytes(1)

//
/// Memory
//

struct Arena {
    U8 *mem;
    U64 cap;
    U64 alloc_pos;
    U64 commit_pos;
};

//
/// String
//

struct String {
    U64 size;
    U8 *data;
};

String * string_alloc(Arena * arena, U64 size);
String * string_make(Arena * arena, const char * raw);
String * string_make(Arena * arena, const char * raw, S32 num);
String * string_concat(Arena * arena, String *str1, String *str2);
String * string_concat(Arena * arena, String *str1, const char * raw);
B8 string_compare(const char * raw1, const char * raw2);
void string_print(String * str);

Arena * arena_init();
U8 * arena_alloc(Arena * arena, U64 size);
