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
        cmd = string_concat(arena, cmd, "  Run with codeclap"); 
        string_print(cmd);
        return 0;
    }
    cmd = string_concat(arena, cmd, "clang++ main.cpp ");
    cmd = string_concat(arena, cmd, "-g -fsanitize=address -static-libsan ");
    cmd = string_concat(arena, cmd,
        "-std=c++20 -Werror -Wall -Wextra -Wshadow -Wconversion "
        "-Wno-unused-variable -Wno-unused-parameter -Wno-deprecated-declarations "
        "-ferror-limit=4 "
        "-I includes "
        "-framework Metal -framework Foundation -framework Cocoa -framework CoreGraphics -framework MetalKit "
        "-O1 "
        "-o temp/main"
    );
    if(modes & mode_codeclap) {
        printf("codeclap active\n");
        cmd = string_concat(arena, cmd, 
            " && time MallocNanoZone=0 codeclap ./temp/main\0"
        );
    } else {
        cmd = string_concat(arena, cmd, 
            " && time MallocNanoZone=0 ./temp/main\0"
        );
    }
    string_print(cmd);
    return system((const char *) cmd->data);
}
