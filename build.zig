const std = @import("std");

const c_sources = [_][]const u8{
    "src/pocketfft.c",
    "testing/tests.c",
};

const common_cflags = [_][]const u8{
    "-std=c23",
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Werror",
};

const sanitize_cflags = common_cflags ++ [_][]const u8{
    "-fsanitize=address,undefined,leak",
    "-g",
};

fn createTestModule(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cflags: []const []const u8,
) *std.Build.Module {
    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    mod.addCSourceFiles(.{
        .files = &c_sources,
        .flags = cflags,
    });
    mod.addIncludePath(b.path("include"));
    mod.linkSystemLibrary("m", .{});
    return mod;
}

fn detectSanitizerLibraryDir(b: *std.Build) ?[]const u8 {
    const cc = b.graph.env_map.get("CC") orelse "cc";
    const libasan_path_raw = b.run(&.{ cc, "-print-file-name=libasan.so" });
    const libasan_path = std.mem.trim(u8, libasan_path_raw, " \t\r\n");
    if (libasan_path.len == 0 or std.mem.eql(u8, libasan_path, "libasan.so")) {
        return null;
    }
    return std.fs.path.dirname(libasan_path);
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const root_mod = createTestModule(b, target, optimize, &common_cflags);

    const exe = b.addExecutable(.{
        .name = "pocketfft_test",
        .root_module = root_mod,
    });
    b.installArtifact(exe);

    const release_mod = createTestModule(b, target, .ReleaseFast, &common_cflags);

    const release_exe = b.addExecutable(.{
        .name = "pocketfft_test_release",
        .root_module = release_mod,
    });

    const sanitize_mod = createTestModule(b, target, .Debug, &sanitize_cflags);
    if (detectSanitizerLibraryDir(b)) |sanitizer_lib_dir| {
        sanitize_mod.addLibraryPath(.{ .cwd_relative = sanitizer_lib_dir });
    }
    sanitize_mod.linkSystemLibrary("asan", .{ .use_pkg_config = .no });
    sanitize_mod.linkSystemLibrary("ubsan", .{ .use_pkg_config = .no });
    sanitize_mod.linkSystemLibrary("lsan", .{ .use_pkg_config = .no });

    const sanitize_exe = b.addExecutable(.{
        .name = "pocketfft_test_sanitize",
        .root_module = sanitize_mod,
    });
    sanitize_exe.bundle_compiler_rt = true;
    sanitize_exe.bundle_ubsan_rt = true;

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    run_cmd.setName("pocketfft_test");
    run_cmd.setEnvironmentVariable("LSAN_OPTIONS", "detect_leaks=1");

    const run_step = b.step("test", "Run pocketfft test binary");
    run_step.dependOn(&run_cmd.step);

    const release_step = b.step("release", "Build optimized pocketfft test binary");
    release_step.dependOn(&b.addInstallArtifact(release_exe, .{}).step);

    const sanitize_run_cmd = b.addRunArtifact(sanitize_exe);
    sanitize_run_cmd.step.dependOn(&b.addInstallArtifact(sanitize_exe, .{}).step);
    sanitize_run_cmd.setName("pocketfft_test_sanitize");
    sanitize_run_cmd.setEnvironmentVariable("LSAN_OPTIONS", "detect_leaks=1");

    const sanitize_step = b.step("sanitize", "Build and run sanitizer-instrumented pocketfft test binary");
    sanitize_step.dependOn(&sanitize_run_cmd.step);
}
