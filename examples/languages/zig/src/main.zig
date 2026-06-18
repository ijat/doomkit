// =============================================================================
//  examples/languages/zig/src/main.zig
// -----------------------------------------------------------------------------
//  Drive the doomkit shared library from Zig using its built-in C FFI.
//  No third-party packages required. GPLv2. See LICENSE.
// =============================================================================
//
//  Zig speaks C directly: declare the library's functions as `extern`, describe
//  the callback struct as an `extern struct` of `callconv(.C)` function
//  pointers, and write the callbacks as plain `callconv(.C)` functions. The
//  build script (build.zig) handles the linking.
//
//  Tested against Zig 0.14. (On Zig 0.15+ the calling-convention literal became
//  lowercase -- replace every `callconv(.C)` with `callconv(.c)`.)
//
//  RUN (after building libdoomgeneric -- see bindings/README.md):
//      put the library where build.zig points (./lib), then:
//      zig build run -- -iwad doom1.wad
// =============================================================================

const std = @import("std");

// The callback struct, field-for-field identical to C's dg_callbacks.
const DgCallbacks = extern struct {
    init: *const fn () callconv(.C) void,
    draw_frame: *const fn () callconv(.C) void,
    sleep_ms: *const fn (ms: u32) callconv(.C) void,
    get_ticks_ms: *const fn () callconv(.C) u32,
    get_key: *const fn (pressed: *c_int, doom_key: *u8) callconv(.C) c_int,
    set_window_title: *const fn (title: ?[*:0]const u8) callconv(.C) void,
};

// The library's exported functions.
extern fn dg_set_callbacks(cb: *const DgCallbacks) void;
extern fn dg_create(argc: c_int, argv: [*]const ?[*:0]const u8) void;
extern fn dg_tick() void;
extern fn dg_screen_buffer() [*]u32;
extern fn dg_resx() c_int;
extern fn dg_resy() c_int;

// Simple process-wide state for the demo (single-threaded, so globals are ok).
var start_ms: i64 = 0;
var frame: i32 = 0;

fn onInit() callconv(.C) void {
    start_ms = std.time.milliTimestamp();
    std.debug.print("[zig] init: framebuffer {d}x{d}\n", .{ dg_resx(), dg_resy() });
}

fn onDrawFrame() callconv(.C) void {
    frame += 1;
    if (frame != 100) return; // save frame #100 to prove pixels flowed

    const w: usize = @intCast(dg_resx());
    const h: usize = @intCast(dg_resy());
    const pixels = dg_screen_buffer()[0 .. w * h];

    writePpm(w, h, pixels) catch |e| {
        std.debug.print("[zig] failed to write frame.ppm: {s}\n", .{@errorName(e)});
        return;
    };
    std.debug.print("[zig] wrote frame.ppm at frame {d}\n", .{frame});
}

fn writePpm(w: usize, h: usize, pixels: []const u32) !void {
    const file = try std.fs.cwd().createFile("frame.ppm", .{});
    defer file.close();

    var buf = std.io.bufferedWriter(file.writer());
    const out = buf.writer();

    try out.print("P6\n{d} {d}\n255\n", .{ w, h });
    for (pixels) |p| {
        const rgb = [3]u8{
            @truncate(p >> 16), // R
            @truncate(p >> 8), // G
            @truncate(p), // B
        };
        try out.writeAll(&rgb);
    }
    try buf.flush();
}

fn onSleepMs(ms: u32) callconv(.C) void {
    std.time.sleep(@as(u64, ms) * std.time.ns_per_ms);
}

fn onGetTicksMs() callconv(.C) u32 {
    return @intCast(std.time.milliTimestamp() - start_ms);
}

fn onGetKey(pressed: *c_int, doom_key: *u8) callconv(.C) c_int {
    _ = pressed;
    _ = doom_key;
    return 0; // no input in this headless demo
}

fn onSetTitle(title: ?[*:0]const u8) callconv(.C) void {
    if (title) |t| std.debug.print("[zig] title: {s}\n", .{t});
}

const callbacks = DgCallbacks{
    .init = onInit,
    .draw_frame = onDrawFrame,
    .sleep_ms = onSleepMs,
    .get_ticks_ms = onGetTicksMs,
    .get_key = onGetKey,
    .set_window_title = onSetTitle,
};

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const alloc = gpa.allocator();

    dg_set_callbacks(&callbacks);

    // Build argv: program name + user args, as a C char**.
    const sys_args = try std.process.argsAlloc(alloc);
    defer std.process.argsFree(alloc, sys_args);

    var argv = std.ArrayList(?[*:0]const u8).init(alloc);
    defer argv.deinit();
    try argv.append("doomdemo");
    for (sys_args[1..]) |a| try argv.append(a.ptr); // forward our args

    dg_create(@intCast(argv.items.len), argv.items.ptr);

    var i: usize = 0;
    while (i < 200) : (i += 1) dg_tick();

    std.debug.print("[zig] done.\n", .{});
}
