const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const exe = b.addExecutable(.{
        .name = "win_main",
        .target = target,
    });
    exe.addIncludePath(.{ .path = "includes" });
    exe.addCSourceFiles(&.{
        "win_main.cpp",
    }, &.{
        "-g",
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
        "-Wno-unknown-pragmas",
        "-ferror-limit=4",
        "-O1",
        "-o temp/main",
    });
    exe.linkLibC();
    exe.linkLibCpp();
    exe.linkSystemLibrary("d3d11");
    exe.linkSystemLibrary("c++");
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
