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
            "src/pocketfft.c",
            "examples/ffttest.c",
        },
        .flags = &.{
            "-std=c23",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
        },
    });
    root_mod.addIncludePath(b.path("include"));
    root_mod.linkSystemLibrary("m", .{});

    const exe = b.addExecutable(.{
        .name = "pocketfft_test",
        .root_module = root_mod,
    });
    b.installArtifact(exe);

    const release_mod = b.createModule(.{
        .target = target,
        .optimize = .ReleaseFast,
        .link_libc = true,
    });
    release_mod.addCSourceFiles(.{
        .files = &.{
            "src/pocketfft.c",
            "examples/ffttest.c",
        },
        .flags = &.{
            "-std=c23",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
        },
    });
    release_mod.addIncludePath(b.path("include"));
    release_mod.linkSystemLibrary("m", .{});

    const release_exe = b.addExecutable(.{
        .name = "pocketfft_test_release",
        .root_module = release_mod,
    });

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    run_cmd.setName("pocketfft_test");
    // Sanitizers can be re-enabled by adding flags above; keep env available for future use.
    run_cmd.setEnvironmentVariable("LSAN_OPTIONS", "log_threads=1:verbosity=1");

    const run_step = b.step("test", "Run pocketfft test binary");
    run_step.dependOn(&run_cmd.step);

    const release_step = b.step("release", "Build optimized pocketfft test binary");
    release_step.dependOn(&b.addInstallArtifact(release_exe, .{}).step);
}
