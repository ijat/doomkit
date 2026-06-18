// build.zig -- tell `zig build` how to compile and link libdoomgeneric.
//
// Adjust the library path below if you put the shared library somewhere else.
// By default we look in the `lib/` folder next to this file (same convention as
// the Rust example). Tested against Zig 0.14.

const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "doomdemo",
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Find and link libdoomgeneric from ./lib, and embed an rpath so the
    // dynamic loader finds it at runtime.
    exe.addLibraryPath(b.path("lib"));
    exe.addRPath(b.path("lib"));
    exe.linkSystemLibrary("doomgeneric");
    exe.linkLibC();

    b.installArtifact(exe);

    // `zig build run -- -iwad doom1.wad`
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);

    const run_step = b.step("run", "Build and run the demo");
    run_step.dependOn(&run_cmd.step);
}
