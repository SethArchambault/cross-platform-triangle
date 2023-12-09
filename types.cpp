#include <stdint.h>

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

/// String

struct String {
    U64 size;
    U8 *data;
};

