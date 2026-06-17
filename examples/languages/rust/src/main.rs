// =============================================================================
//  examples/languages/rust/src/main.rs
// -----------------------------------------------------------------------------
//  Drive the genericdoom shared library from Rust using its built-in C FFI
//  (extern "C"). No crates required. GPLv2. See LICENSE.
// =============================================================================
//
//  Rust's FFI is direct: declare the C functions in an `extern "C"` block, write
//  the callbacks as `extern "C" fn`, and link the library (see build.rs).
//
//  RUN (after building libdoomgeneric -- see bindings/README.md):
//      put the library where build.rs points (./lib), then:
//      cargo run -- -iwad doom1.wad
// =============================================================================

use std::ffi::CString;
use std::fs::File;
use std::io::Write;
use std::os::raw::{c_char, c_int, c_uchar};
use std::time::Instant;

// The callback struct, field-for-field identical to C's dg_callbacks.
#[repr(C)]
struct DgCallbacks {
    init: extern "C" fn(),
    draw_frame: extern "C" fn(),
    sleep_ms: extern "C" fn(u32),
    get_ticks_ms: extern "C" fn() -> u32,
    get_key: extern "C" fn(*mut c_int, *mut c_uchar) -> c_int,
    set_window_title: extern "C" fn(*const c_char),
}

// The library's exported functions.
extern "C" {
    fn dg_set_callbacks(cb: *const DgCallbacks);
    fn dg_create(argc: c_int, argv: *const *const c_char);
    fn dg_tick();
    fn dg_screen_buffer() -> *mut u32;
    fn dg_resx() -> c_int;
    fn dg_resy() -> c_int;
}

// Simple process-wide state for the demo (single-threaded, so a static mut is ok
// here; a real app would use a safer cell/atomic).
static mut FRAME: i32 = 0;
static mut START: Option<Instant> = None;

extern "C" fn on_init() {
    unsafe {
        START = Some(Instant::now());
        println!("[rust] init: framebuffer {}x{}", dg_resx(), dg_resy());
    }
}

extern "C" fn on_draw_frame() {
    unsafe {
        FRAME += 1;
        let frame = FRAME; // copy out of the static so we don't reference it
        if frame != 100 {
            return;
        }
        let (w, h) = (dg_resx() as usize, dg_resy() as usize);
        let pixels = std::slice::from_raw_parts(dg_screen_buffer(), w * h);
        if let Ok(mut f) = File::create("frame.ppm") {
            let _ = write!(f, "P6\n{} {}\n255\n", w, h);
            let mut rgb = Vec::with_capacity(w * h * 3);
            for &p in pixels {
                rgb.push((p >> 16) as u8); // R
                rgb.push((p >> 8) as u8); // G
                rgb.push(p as u8); // B
            }
            let _ = f.write_all(&rgb);
            println!("[rust] wrote frame.ppm at frame {}", frame);
        }
    }
}

extern "C" fn on_sleep_ms(ms: u32) {
    std::thread::sleep(std::time::Duration::from_millis(ms as u64));
}

extern "C" fn on_get_ticks_ms() -> u32 {
    unsafe { START.map(|s| s.elapsed().as_millis() as u32).unwrap_or(0) }
}

extern "C" fn on_get_key(_pressed: *mut c_int, _key: *mut c_uchar) -> c_int {
    0 // no input in this headless demo
}

extern "C" fn on_set_title(title: *const c_char) {
    if !title.is_null() {
        let s = unsafe { std::ffi::CStr::from_ptr(title) };
        println!("[rust] title: {}", s.to_string_lossy());
    }
}

fn main() {
    let cb = DgCallbacks {
        init: on_init,
        draw_frame: on_draw_frame,
        sleep_ms: on_sleep_ms,
        get_ticks_ms: on_get_ticks_ms,
        get_key: on_get_key,
        set_window_title: on_set_title,
    };
    unsafe { dg_set_callbacks(&cb) };

    // Build argv: program name + user args, as a C char**.
    let args: Vec<CString> = std::iter::once("doomdemo".to_string())
        .chain(std::env::args().skip(1))
        .map(|a| CString::new(a).unwrap())
        .collect();
    let argv: Vec<*const c_char> = args.iter().map(|a| a.as_ptr()).collect();

    unsafe {
        dg_create(argv.len() as c_int, argv.as_ptr());
        for _ in 0..200 {
            dg_tick();
        }
    }
    println!("[rust] done.");
}
