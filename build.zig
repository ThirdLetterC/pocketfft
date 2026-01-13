const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const root_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    root_mod.addCSourceFiles(.{
        .files = &.{
            "pocketfft.c",
            "ffttest.c",
        },
        .flags = &.{
            "-std=c2x",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
        },
    });
    root_mod.addIncludePath(b.path("."));
    root_mod.linkSystemLibrary("m", .{});

    const exe = b.addExecutable(.{
        .name = "pocketfft_test",
        .root_module = root_mod,
    });
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    run_cmd.setName("pocketfft_test");
    // Sanitizers can be re-enabled by adding flags above; keep env available for future use.
    run_cmd.setEnvironmentVariable("LSAN_OPTIONS", "log_threads=1:verbosity=1");

    const run_step = b.step("test", "Run pocketfft test binary");
    run_step.dependOn(&run_cmd.step);
}
