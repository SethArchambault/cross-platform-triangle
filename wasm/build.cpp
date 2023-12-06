#if 0
    /usr/bin/clang++ -std=c++11 build.cpp -g -fsanitize=address -O0 -o temp/build && 
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
    mode_run        = 0x2,
    mode_server     = 0x4,
};

S32 command_run(Arena *arena, S32 mode);

int main(S32 argc, char ** argv) 
{
    Arena *arena = arena_init();
    S32 mode = mode_run;

    for(S32 idx = 1; idx < argc; ++idx) 
    {
        if (string_compare(argv[idx], "help")) {
            // overwrite mode_run   
            mode = mode_help;
        }
        if (string_compare(argv[idx], "server")) {
            // overwrite mode_run   
            mode = mode_server;
        }
    }

    S32 ret = command_run(arena, mode);
    if (ret) {
        command_run(arena, mode_help);
    }
    return 0;
}


S32 command_run(Arena *arena, S32 mode)
{
    String * cmd = string_make(arena, "");

    if (mode & mode_help) {
        cmd = string_concat(arena, cmd, "Before running ./build.cpp, run:\n"); 
        cmd = string_concat(arena, cmd, "emsdk\n"); 
        cmd = string_concat(arena, cmd, "   This runs emsdk, to make emcc available. Run once.\n"); 
        string_print(cmd);
        return 0;
    }
    if (mode & mode_run ) {
        cmd = string_concat(arena, cmd, R"RAW(
            emcc \
            -s TOTAL_MEMORY=536870912\
            --shell-file template.html\
            -sMAX_WEBGL_VERSION=2\
            -sMIN_WEBGL_VERSION=2\
            -Wno-deprecated-declarations -Wno-#warnings \
            -s WASM=1 -s USE_WEBGL2=1 -s USE_GLFW=3\
            -s STB_IMAGE=1\
            -l GLEW -l glfw3 -L/usr/local/lib\
            main.cpp -o index.html
)RAW"
        );
    }
    if (mode & mode_server ) {
        cmd = string_concat(arena, cmd, R"RAW(
            php -S localhost:8080
)RAW"
        );
    }
    string_print(cmd);
    return system((const char *) cmd->data);
}


