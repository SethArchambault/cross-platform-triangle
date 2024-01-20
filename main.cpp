#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

#define BUFFER_MAX  50

#ifdef DEBUG_HASH
#define debug_hash printf
#else
#define debug_hash //
#endif

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
// previously U64
static S32 hash_key(String *key) {
    U64 hash = FNV_OFFSET;
    for (U64 idx = 0; idx < key->size; idx++) {
        hash ^= (U64)(unsigned char)(*(key->data + idx));
        hash *= FNV_PRIME;
    }
    return (S32) (hash % BUFFER_MAX);
}

void draw_arrow(String * id_str, V2F32 pos, F32 width, F32 length, F32 turns, V3F32 color) {
    V2F32 p1;
    V2F32 p2;
    V2F32 p3;
    F32 degrees = turns * 360.0f;
    {
        F32 hypotenuse = width / 2.0f;
        F32 radians = degrees * (F32) M_PI / 180.0f;
        F32 adjacent = cos(radians) * hypotenuse;
        F32 opposite = sin(radians) * hypotenuse;

        // lower left
        p1 = {pos.x - adjacent, pos.y - opposite};
        // lower right
        p2 = {pos.x + adjacent, pos.y + opposite};
    }
    {
        F32 hypotenuse = length;
        F32 radians = degrees * (F32) M_PI / 180;
        F32 adjacent = cos(radians) * hypotenuse;
        F32 opposite = sin(radians) * hypotenuse;
        // upper center
        p3 = {pos.x + opposite, pos.y - adjacent};
    }
    // code should be in windows_platform.cpp
    platform_draw_triangle(id_str, p1, p2, p3, color);
}


struct State {
    B32 initialized;
    Arena * arena;
    F32 rotation;
};
State __s;



void game_mouse_move(S32 x, S32 y) {
    //__s.rotation = (F32) x / 300.0f;
}
S32 game_loop() {
    if (!__s.initialized) {
        __s.arena = arena_init();
        __s.initialized = 1;
        __s.rotation = 0.0f;
    }
    V3F32 color = {{200.0}, {0.0}, {150.0}};
    for (S32 idx = 0; idx < 50; idx++) {
        String *id_str = string_make(__s.arena, "basic_triangle", idx);
        draw_arrow(id_str, {idx*50.0f, idx*25.0f}, 50.0f, 70.0f, __s.rotation+(F32)idx*0.1f, color);
    }
    __s.rotation += 0.01f ;
    if (__s.rotation > 2.0f) {
        __s.rotation = 0.0f;
    }
    printf("rotation %f\n", __s.rotation);
    return 0;
}

