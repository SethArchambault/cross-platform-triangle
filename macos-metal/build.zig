const std = @import("std");

pub fn build(b: *std.Build) void {
    const exe = b.addExecutable(.{
        .name = "main",
    });
    //exe.addIncludePath("includes");
    //exe.addIncludeDir("includes");
    exe.addIncludePath(.{ .path = "includes" });
    exe.addCSourceFiles(&.{
        "main.cpp",
    }, &.{
        "-g",
        //"-fsanitize=address",
        "-fno-sanitize=undefined", // undefined behavior crash is hit otherwise..
        //        "-static-libsan",
        "-std=c++20",
        "-Werror",
        "-Wall",
        "-Wextra",
        "-Wshadow",
        "-Wconversion",
        "-Wno-unused-variable",
        "-Wno-unused-parameter",
        "-Wno-deprecated-declarations",
        "-Wno-unused-value",
        //        "-ferror-limit=4",
        "-O1",
        "-o temp/main",
    });
    exe.linkLibC();
    exe.linkLibCpp();
    exe.linkSystemLibrary("c++");
    exe.linkFramework("Metal");
    exe.linkFramework("Foundation");
    exe.linkFramework("Cocoa");
    exe.linkFramework("CoreGraphics");
    exe.linkFramework("MetalKit");
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    // `zig build run -- arg1 arg2 etc`
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
