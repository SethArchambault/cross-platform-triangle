#include <string.h> // memcpy

String * string_alloc(Arena * arena, U64 size) {
    arena_alloc(arena, size + 64);
    String * str = (String *) (arena->mem + arena->alloc_pos);
    str->size  = size;
    str->data  = arena->mem + arena->alloc_pos + 64;
    return str;
}

String * string_make(Arena * arena, const char * raw) {
    U64 str_size = (U64) strlen(raw);
    String * str = string_alloc(arena, str_size);
    memcpy(str->data, raw, str_size);
    return str;
}

String * string_make(Arena * arena, const char * raw, S32 num) {
    U64 str_len = (U64) strlen(raw);
    char num_raw[100];
    sprintf(num_raw, "%d", num);
    U64 num_len = (U64) strlen(num_raw);
    String * str = string_alloc(arena, str_len + num_len);
    memcpy(str->data, raw, str_len);
    memcpy(str->data+str_len, num_raw, num_len);
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

String * string_concat(Arena * arena, String *str1, char raw) {
    U64 str3_size = str1->size + 1;
    String * str3 = string_alloc(arena, str3_size);
    str3->size = str3_size;
    memcpy(str3->data, str1->data, str1->size);
    str3->data[str1->size] = (U8)raw;
    return str3;
}

B8 string_compare(String *raw1, String *raw2) {
    if (raw1->size != raw2->size) return false;
    for(U32 idx = 0; idx < raw1->size; ++idx) {
        if (raw1->data[idx] != raw2->data[idx]) return false;
    }
    return true;
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
    printf("%.*s\n", (int)str->size, (char *)str->data);
}




