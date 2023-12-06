///usr/bin/clang++ build.cpp -g -fsanitize=address -O0 -o temp/build && ./temp/build $*; rm temp/build; exit

#include<stdio.h>
#include <stdlib.h>
#include "../common.cpp"

enum {
    mode_none       = 0x0, 
    mode_help       = 0x1,
    mode_list       = 0x2,
    mode_device     = 0x4,
};

S32 command_run(Arena *arena, S32 modes, String* device_name);

int main(int argc, char ** argv) {
    Arena *arena = arena_init();
    S32 modes = mode_none;
    String *device_name = string_make(arena, "");
    for(S32 idx = 1; idx < argc; ++idx) {
        if (string_compare(argv[idx], "list")) 
        {
            modes |= mode_list;
        } 
        else if (string_compare(argv[idx], "device")) 
        {
            modes |= mode_device;
            ++idx;
            if (idx < argc) {
                device_name = string_concat(arena, device_name, argv[idx]);
                device_name = string_concat(arena, device_name, " ");
            }
            continue;
        }  
        if (string_compare(argv[idx], "help")) 
        {
            modes = mode_help;
        }
    }

    S32 ret = command_run(arena, modes, device_name);
    if (ret) {
        command_run(arena, mode_help, device_name);
    }
    return 0;
}

S32 command_run(Arena *arena, S32 modes, String* device_name)
{
    String * cmd = string_make(arena, "");

    cmd = string_concat(arena, cmd, 
            "xcodebuild -workspace IosMetalTriangle.xcworkspace -configuration Debug -scheme 01-primitive ");
    if (modes & mode_help) {
        cmd = string_concat(arena, cmd, "\nOptions:\n"); 
        cmd = string_concat(arena, cmd, "./build.cpp list\n"); 
        cmd = string_concat(arena, cmd, "./build.cpp device"); 
        string_print(cmd);
        return 0;
    }
    if (modes & mode_list) {
        cmd = string_concat(arena, cmd, "-showdestinations ");
    } 
    if (modes & mode_device) {
        cmd = string_concat(arena, cmd, "-destination \"");
        cmd = string_concat(arena, cmd, device_name);
        cmd = string_concat(arena, cmd, "\" ");
    }
    else 
    {
        cmd = string_concat(arena, cmd, "-destination \"name=iPhone 15\" ");
    }
    cmd = string_concat(arena, cmd, "clean build install");
    string_print(cmd);
    return system((const char *) cmd->data);
}

