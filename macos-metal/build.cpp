//usr/bin/clang++ build.cpp -g -fsanitize=address -Wno-deprecated-declarations -O0 -o temp/build && ./temp/build $*; rm temp/build;exit;

#include<stdio.h>
#include <stdlib.h>
#include "../common.h"
#include "../macos_platform.cpp"
#include "../common.cpp"

enum {
    mode_none       = 0x0, 
    mode_help       = 0x1,
    mode_codeclap   = 0x2,
    mode_nopoison     = 0x4,
    mode_debug_hash = 0x8,
};

S32 command_run(Arena *arena, S32 modes);

int main(int Argc, char ** Argv) {
    U32 modes = 0;

    for (int Idx = 1; Idx < Argc; ++Idx) {
        if(string_compare("codeclap", Argv[Idx])) {
            modes |= mode_codeclap; 
        }
        if(string_compare("help", Argv[Idx])) {
            modes |= mode_help; 
        }
        if(string_compare("nopoison", Argv[Idx])) {
            modes |= mode_nopoison; 
        }
        if(string_compare("debug_hash", Argv[Idx])) {
            modes |= mode_debug_hash; 
        }
    }
    const char str[] = "test";
    Arena *arena = arena_init();

    S32 ret = command_run(arena, modes);
    if (ret) 
    {
        command_run(arena, mode_help);
    }
    return 0;
}

S32 command_run(Arena *arena, S32 modes)
{
    String * cmd = string_make(arena, "");
    if (modes & mode_help) {
        cmd = string_concat(arena, cmd, "Options:\n"); 
        cmd = string_concat(arena, cmd, "./build.cpp codeclap\n"); 
        cmd = string_concat(arena, cmd, "  Run with codeclap\n"); 
        cmd = string_concat(arena, cmd, "./build.cpp debug_hash\n"); 
        cmd = string_concat(arena, cmd, "  show debug hash messages\n"); 
        cmd = string_concat(arena, cmd, "./build.cpp nopoison\n"); 
        cmd = string_concat(arena, cmd, "  disable memory poisoning\n"); 
        string_print(cmd);
        return 0;
    }
    cmd = string_concat(arena, cmd, "time clang++ macos_main.cpp ");
    cmd = string_concat(arena, cmd, "-g -fsanitize=address -static-libsan ");
    if (!(modes & mode_nopoison)) {
        cmd = string_concat(arena, cmd, "-DPOISON "); 
    }
    if (modes & mode_debug_hash) {
        cmd = string_concat(arena, cmd, "-DDEBUG_HASH "); 
    }
    cmd = string_concat(arena, cmd,
        "-fsanitize=undefined "
        "-fno-sanitize-recover=all " // crashes with message
        "-std=c++20 -Werror -Wall -Wextra -Wshadow -Wconversion "
        "-Wno-unused-variable -Wno-unused-parameter -Wno-deprecated-declarations -Wno-unused-value "
        "-ferror-limit=4 "
        "-I includes "
        "-framework Metal -framework Foundation -framework Cocoa -framework CoreGraphics -framework MetalKit "
        "-O1 "
        "-o temp/main"
    );
    if(modes & mode_codeclap) {
        printf("codeclap active\n");
        cmd = string_concat(arena, cmd, 
            " && MallocNanoZone=0 codeclap ./temp/main"
        );
    } 
    else {
        cmd = string_concat(arena, cmd, 
            " && MallocNanoZone=0 ./temp/main"
        );
    }

    string_print(cmd);

    cmd = string_concat(arena, cmd, '\0');
    return system((const char *) cmd->data);
}
