#if 0
    /usr/bin/clang++ build.cpp -g -fsanitize=address -O0 -o temp/build && 
    ./temp/build $*
    rm temp/build 
    exit
#endif

#include<stdio.h>
#include <stdlib.h>
#include "../common.cpp"

enum {
    mode_none       = 0x0, 
    mode_help       = 0x1,
    mode_device     = 0x2,
    mode_install    = 0x4,
    mode_list       = 0x8,
};

S32 command_run(Arena *arena, S32 modes, String* device_name);

int main(S32 argc, char ** argv) 
{
    Arena *arena = arena_init();
    S32 modes = mode_install;

    // default
    String * device_name = string_make(arena, "");
    for(S32 idx = 1; idx < argc; ++idx) 
    {
        if (string_compare(argv[idx], "device")) 
        {
            modes |= mode_device;
            idx++;
            if (idx < argc) 
            {
                device_name = string_concat(arena, device_name, argv[idx]);
                device_name = string_concat(arena, device_name, " ");
            }
            else 
            {
                device_name = string_concat(arena, device_name, "Pixel_4_API_30 ");
            }
            continue;
        }
        if (string_compare(argv[idx], "list")) {
            // overwrite mode_install
            modes = mode_list;
        }
        if (string_compare(argv[idx], "help")) {
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

    if (modes & mode_help) {
        cmd = string_concat(arena, cmd, "Options:\n"); 
        cmd = string_concat(arena, cmd, "./build.cpp list\n"); 
        cmd = string_concat(arena, cmd, "./build.cpp device"); 
        string_print(cmd);
        return 0;
    }
    if (modes & mode_list) {
        cmd = string_concat(arena, cmd, "emulator -list-avds"); 
    } 
    if (modes & mode_device) {
        cmd = string_concat(arena, cmd, "emulator -avd ");
        cmd = string_concat(arena, cmd, device_name);
        cmd = string_concat(arena, cmd, " & sleep 2 && ");
    }
    if (modes & mode_install) {
        cmd = string_concat(arena, cmd, "./gradlew installDebug");
    }
    string_print(cmd);
    return system((const char *) cmd->data);
}
